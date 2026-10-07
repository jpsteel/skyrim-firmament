#include "Hooks.h"
#include "Utility.h"
#include "Skills.h"
#include "Lookup.h"
#include "RE\Offsets.h"
#include "Exploration.h"
#include "Philosophy.h"

float lastWarmthValue = 0.0f;
float lastFatigueValue = 0.0f;
float lastHungerValue = 0.0f;
bool firstWarmthCheck = true;
bool firstFatigueCheck = true;
bool firstHungerCheck = true;

namespace {
    using ConditionFn = RE::SCRIPT_FUNCTION::Condition_t;

    ConditionFn* originalAdvanceObjectHasKeyword = nullptr;

    bool IsReadableProtection(DWORD protect) {
        if ((protect & PAGE_GUARD) != 0 || (protect & PAGE_NOACCESS) != 0) {
            return false;
        }

        switch (protect & 0xFF) {
            case PAGE_READONLY:
            case PAGE_READWRITE:
            case PAGE_WRITECOPY:
            case PAGE_EXECUTE_READ:
            case PAGE_EXECUTE_READWRITE:
            case PAGE_EXECUTE_WRITECOPY:
                return true;
            default:
                return false;
        }
    }

    bool IsExecutableProtection(DWORD protect) {
        if ((protect & PAGE_GUARD) != 0 || (protect & PAGE_NOACCESS) != 0) {
            return false;
        }

        switch (protect & 0xFF) {
            case PAGE_EXECUTE:
            case PAGE_EXECUTE_READ:
            case PAGE_EXECUTE_READWRITE:
            case PAGE_EXECUTE_WRITECOPY:
                return true;
            default:
                return false;
        }
    }

    bool IsReadableRange(const void* ptr, std::size_t size) {
        if (!ptr || size == 0) {
            return false;
        }

        MEMORY_BASIC_INFORMATION mbi{};

        if (::VirtualQuery(ptr, std::addressof(mbi), sizeof(mbi)) == 0) {
            return false;
        }

        if (mbi.State != MEM_COMMIT || !IsReadableProtection(mbi.Protect)) {
            return false;
        }

        const auto begin = reinterpret_cast<std::uintptr_t>(ptr);

        const auto regionBegin = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress);

        const auto regionEnd = regionBegin + mbi.RegionSize;

        return begin >= regionBegin && begin < regionEnd && size <= regionEnd - begin;
    }

    bool IsExecutableAddress(const void* ptr) {
        if (!ptr) {
            return false;
        }

        MEMORY_BASIC_INFORMATION mbi{};

        if (::VirtualQuery(ptr, std::addressof(mbi), sizeof(mbi)) == 0) {
            return false;
        }

        return mbi.State == MEM_COMMIT && IsExecutableProtection(mbi.Protect);
    }

    bool IsValidTESFormPointer(const void* ptr) {
        constexpr std::uintptr_t kMinimumUserPointer = 0x10000;
        constexpr std::size_t kTESFormBytesNeeded = 0x1B;

        if (!ptr) {
            return false;
        }

        const auto address = reinterpret_cast<std::uintptr_t>(ptr);

        if (address < kMinimumUserPointer) {
            return false;
        }

        if (!IsReadableRange(ptr, kTESFormBytesNeeded)) {
            return false;
        }

        const auto vtable = *reinterpret_cast<void* const*>(ptr);

        if (reinterpret_cast<std::uintptr_t>(vtable) < kMinimumUserPointer || !IsReadableRange(vtable, sizeof(void*))) {
            return false;
        }

        const auto firstVirtual = *reinterpret_cast<void* const*>(vtable);

        if (reinterpret_cast<std::uintptr_t>(firstVirtual) < kMinimumUserPointer ||
            !IsExecutableAddress(firstVirtual)) {
            return false;
        }

        const auto rawFormType = *(reinterpret_cast<const std::uint8_t*>(ptr) + 0x1A);

        return rawFormType < static_cast<std::uint8_t>(RE::FormType::Max);
    }

    bool IsFirmamentSkillKeyword(const void* param1) {
        if (!IsValidTESFormPointer(param1)) {
            return false;
        }

        const auto* form = static_cast<const RE::TESForm*>(param1);

        if (form->GetFormType() != RE::FormType::Keyword) {
            return false;
        }

        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            return false;
        }

        const auto formID = form->GetFormID();

        const auto horseman = dataHandler->LookupFormID(0x34, "FirmamentNewSkills.esp");

        const auto exploration = dataHandler->LookupFormID(0x35, "FirmamentNewSkills.esp");

        const auto philosophy = dataHandler->LookupFormID(0x36, "FirmamentNewSkills.esp");

        return formID == horseman || formID == exploration || formID == philosophy;
    }

    bool AdvanceObjectHasKeywordHook(RE::TESObjectREFR* subject, void* param1, void* param2, double& result) {
        auto* player = RE::PlayerCharacter::GetSingleton();

        if (!player || subject != player || !param1) {
            return originalAdvanceObjectHasKeyword(subject, param1, param2, result);
        }

        if (!IsFirmamentSkillKeyword(param1)) {
            return originalAdvanceObjectHasKeyword(subject, param1, param2, result);
        }

        auto* advanceObject = player->GetInfoRuntimeData().advanceObject;

        if (!IsValidTESFormPointer(advanceObject)) {
            logger::warn(
                "Blocked invalid advanceObject 0x{:X} during Firmament skill-use condition.",
                reinterpret_cast<std::uintptr_t>(advanceObject));

            result = 0.0;
            return true;
        }

        return originalAdvanceObjectHasKeyword(subject, param1, param2, result);
    }
}

void SharedHooks::InstallHooks() {
    auto& trampoline = SKSE::GetTrampoline();

    //Update Hook
    REL::Relocation<uintptr_t> vtbl{RE::VTABLE_PlayerCharacter[0]};
    _Update = vtbl.write_vfunc(173, Update);

    logger::info("Shared hooks successfully installed.");
}

void SharedHooks::InstallSkillUseGuard() {
    auto* command = RE::SCRIPT_FUNCTION::LocateScriptCommand("EPModSkillUsage_AdvanceObjectHasKeyword"sv);

    if (!command || !command->conditionFunction) {
        logger::error(
            "Could not locate EPModSkillUsage_AdvanceObjectHasKeyword.");
        return;
    }

    originalAdvanceObjectHasKeyword = command->conditionFunction;

    ConditionFn* replacement = AdvanceObjectHasKeywordHook;

    const auto slot = reinterpret_cast<std::uintptr_t>(std::addressof(command->conditionFunction));

    REL::safe_write(slot, std::addressof(replacement), sizeof(replacement));

    if (command->conditionFunction != replacement) {
        logger::critical("Failed to install Firmament skill-use crash guard.");

        originalAdvanceObjectHasKeyword = nullptr;
        return;
    }

    logger::info("Firmament skill-use crash guard installed.");
}

void SharedHooks::Update(RE::PlayerCharacter* player, float delta) {
    _Update(player, delta);

    if (player->IsOnMount() && player->AsActorState()->IsSprinting()) {
        logger::trace("Updating while player is on galoping mount.");
        if (const auto customSkills = GetCustomSkillsInterface()) {
            customSkills->AdvanceSkill("Horseman", delta);
            logger::trace("Advancing Horseman Skill.");
        }
    }

    if (Philosophy::inApocrypha) {
        logger::trace("Updating while player is in Apocrypha.");
        if (const auto customSkills = GetCustomSkillsInterface()) {
            customSkills->AdvanceSkill("Philosophy", delta);
            logger::trace("Advancing Philosophy Skill.");
        }
    }

    auto temperatureLevel = RE::TESForm::LookupByEditorID<RE::TESGlobal>("Survival_TemperatureLevel");
    if (temperatureLevel) {
        logger::trace("Current temperature level: {}.", temperatureLevel->value);
        if (temperatureLevel->value >= 3) {
            if (const auto customSkills = GetCustomSkillsInterface()) {
                customSkills->AdvanceSkill("Exploration", delta);
                logger::trace("Advancing Exploration Skill.");
            }
        }
    }

    if (Exploration::initializedCachedValues) {
        auto explorationBonus = player->AsActorValueOwner()->GetActorValue(LookupActorValueByName("ExplorationPotions"));

        auto exhaustionNeedRate = RE::TESForm::LookupByEditorID<RE::TESGlobal>("Survival_ExhaustionNeedRate");
        auto hungerNeedRate = RE::TESForm::LookupByEditorID<RE::TESGlobal>("Survival_HungerNeedRate");
        auto cachedExhaustionNeedRate = RE::TESForm::LookupByEditorID<RE::TESGlobal>("CachedExhaustionNeedRate");
        auto cachedHungerNeedRate = RE::TESForm::LookupByEditorID<RE::TESGlobal>("CachedHungerNeedRate");

        float clampedBonus = std::clamp(explorationBonus, 0.0f, 100.0f);
        float bonusMult = 1.0f - (clampedBonus / 100.0f);
        if (bonusMult <= 0.f) {
            bonusMult = 0.01f;
        }
        hungerNeedRate->value = cachedHungerNeedRate->value * bonusMult;
        exhaustionNeedRate->value = cachedExhaustionNeedRate->value * bonusMult;
        if (Exploration::appliedDungeonDelver) {
            hungerNeedRate->value *= 0.5f;
            exhaustionNeedRate->value *= 0.5f;
        }
    } else {
        Exploration::InitCachedValues();
    }

    Exploration::UpdateCamperWellRested();

    Philosophy::EvaluateHermit();
}