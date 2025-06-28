#include "Hooks.h"
#include "Utility.h"
#include "Skills.h"
#include "Lookup.h"
#include "RE\Offsets.h"
#include "Exploration.h"
#include "Philosophy.h"

#include <xbyak/xbyak.h>

float lastWarmthValue = 0.0f;
float lastFatigueValue = 0.0f;
float lastHungerValue = 0.0f;
bool firstWarmthCheck = true;
bool firstFatigueCheck = true;
bool firstHungerCheck = true;

void SharedHooks::InstallHooks() {
    auto& trampoline = SKSE::GetTrampoline();

    //Update Hook
    REL::Relocation<uintptr_t> vtbl{RE::VTABLE_PlayerCharacter[0]};
    _Update = vtbl.write_vfunc(173, Update);

    logger::info("Shared hooks successfully installed.");
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