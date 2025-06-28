#include "Philosophy.h"
#include "Utility.h"
#include "editorID.hpp"

bool Philosophy::inApocrypha = false;

RE::BSEventNotifyControl Philosophy::EventProcessor::ProcessEvent(
    const RE::MenuOpenCloseEvent* event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* eventSource) {
    if (!event) {
        return RE::BSEventNotifyControl::kContinue;
    }

    if (!event->opening && event->menuName == RE::StatsMenu::MENU_NAME) {
        UpdateAvidReader(false);
        UpdateErudite(false);
        UpdateCultist(false);
        UpdateHermit();
    } else if (!event->opening && event->menuName == RE::DialogueMenu::MENU_NAME) {
        ResetHermitCounter();
    } else if (!event->opening && event->menuName == RE::LoadingMenu::MENU_NAME) {
        auto* player = RE::PlayerCharacter::GetSingleton();
        auto apocryphaWorld = RE::TESForm::LookupByEditorID<RE::TESWorldSpace>("DLC2ApocryphaWorld");
        auto apocryphaLocation = RE::TESForm::LookupByEditorID<RE::BGSLocation>("DLC2ApocryphaLocation");
        if (player->GetWorldspace()) {
            if (player->GetWorldspace() == apocryphaWorld) {
                logger::trace("Player is in Apocrypha.");
                inApocrypha = true;
            } else {
                logger::trace("Player not in Apocrypha.");
                inApocrypha = false;
            }
        } else if (player->GetParentCell() && player->GetParentCell()->GetLocation()) {
            if (player->GetParentCell()->GetLocation()) {
                auto* rootLocation = GetRootLocation(player->GetParentCell()->GetLocation());
                if (rootLocation == apocryphaLocation) {
                    logger::trace("Player is in Apocrypha.");
                    inApocrypha = true;
                } else {
                    logger::trace("Player not in Apocrypha.");
                    inApocrypha = false;
                }
            }
        } else {
            logger::trace("Player not in Apocrypha.");
            inApocrypha = false;
        }
    }

    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl Philosophy::EventProcessor::ProcessEvent(
    const RE::TESCombatEvent* event, RE::BSTEventSource<RE::TESCombatEvent>* eventSource) {
    if (!event || !event->actor || !event->targetActor) {
        return RE::BSEventNotifyControl::kContinue;
    }

    auto* player = RE::PlayerCharacter::GetSingleton();
    auto* targetActor = event->targetActor.get()->As<RE::Actor>();
    if (!targetActor->IsPlayerRef() && !IsPlayersMount(targetActor)) {
        logger::trace("[Linguist] Player not involved in combat.");
        return RE::BSEventNotifyControl::kContinue;
    }

    static auto dwemerKeyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("ActorTypeDwarven");
    static auto npcKeyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("FNS_LinguistTarget");
    if (!event->actor->HasKeyword(npcKeyword)) {
        logger::trace("[Linguist] Actor not a potential target.");
        return RE::BSEventNotifyControl::kContinue;
    }

    static auto linguistPerk = RE::TESForm::LookupByEditorID<RE::BGSPerk>("FNS_PHL_Linguist");
    static auto calmSpell = RE::TESForm::LookupByEditorID<RE::SpellItem>("FNS_PHL_LinguistSpell");
    if (!player->HasPerk(linguistPerk)) {
        logger::trace("[Linguist] Player hasn't unlocked Linguist.");
        return RE::BSEventNotifyControl::kContinue;
    }

    static auto linguistDaysPassed = RE::TESForm::LookupByEditorID<RE::TESGlobal>("LinguistDaysPassed");
    static auto gameDaysPassed = RE::TESForm::LookupByEditorID<RE::TESGlobal>("GameDaysPassed");
    if (linguistDaysPassed->value - gameDaysPassed->value > 0.f) {
        logger::info("[Linguist] Less than one hour has passed since last failed roll.");
        return RE::BSEventNotifyControl::kContinue;
    }
    
    int random = RandomInt(0, 100);
    auto linguistThreshold = RE::TESForm::LookupByEditorID<RE::TESGlobal>("LinguistThreshold")->value;
    logger::info("[Linguist] Random num: {} (<={}?)", random, linguistThreshold);
    if (random > linguistThreshold) {
        logger::info("[Linguist] Failed dice roll!");
        static auto msgRef = RE::TESForm::LookupByEditorID<RE::BGSMessage>("FNS_LinguistMessageFail");
        RE::BSString msg;
        msgRef->GetDescription(msg, msgRef);
        RE::DebugNotification(msg.c_str());
        linguistDaysPassed->value = gameDaysPassed->value + 0.05;
        logger::info("Restarting \"Linguist\" cooldown.");
        return RE::BSEventNotifyControl::kContinue;
    }
    logger::info("[Linguist] Successful dice roll!");

    if (auto caster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant); caster) {
        logger::info("[Linguist] Casting Linguist spell.");
        caster->CastSpellImmediate(calmSpell, false, player, 1.0f, false, 0.0f, player);
        static auto msgRef = RE::TESForm::LookupByEditorID<RE::BGSMessage>("FNS_LinguistMessageSuccess");
        RE::BSString msg;
        msgRef->GetDescription(msg, msgRef);
        RE::DebugNotification(msg.c_str());
    }

    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl Philosophy::EventProcessor::ProcessEvent(
    const RE::TESMagicEffectApplyEvent* event, RE::BSTEventSource<RE::TESMagicEffectApplyEvent>* eventSource) {
    if (!event || !event->magicEffect || !event->target) {
        return RE::BSEventNotifyControl::kContinue;
    }

    if (!event->target->IsPlayerRef()) {
        return RE::BSEventNotifyControl::kContinue;
    }

    auto* player = RE::PlayerCharacter::GetSingleton();
    static auto linguistPerk = RE::TESForm::LookupByEditorID<RE::BGSPerk>("FNS_PHL_Monk");
    if (!player->HasPerk(linguistPerk)) {
        logger::trace("[Monk] Player hasn't unlocked Monk.");
        return RE::BSEventNotifyControl::kContinue;
    }

    ApplyMonkBonus();

    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl Philosophy::EventProcessor::ProcessEvent(
    const RE::TESQuestStageEvent* event, RE::BSTEventSource<RE::TESQuestStageEvent>* eventSource) {
    if (!event) {
        return RE::BSEventNotifyControl::kContinue;
    }
    UpdateCultist(true);

    return RE::BSEventNotifyControl::kContinue;
}

void Philosophy::InstallHooks() {
    ReadBookInventory::Install();
    ReadBookContainer::Install();
    ReadBookReference::Install();

    logger::info("Philosophy hooks successfully installed.");
}

void Philosophy::PhilosophyProcessBookXP(RE::TESObjectBOOK* book) {
    if (!book || book->IsRead()) return;

    float skillXP = book->GetGoldValue() + 50;
    float clampedXP = std::clamp(skillXP, 0.0f, 100.0f);
    if (const auto customSkills = GetCustomSkillsInterface()) {
        float potionBonus = RE::PlayerCharacter::GetSingleton()->AsActorValueOwner()->GetActorValue(
            LookupActorValueByName("PhilosophyPotions"));
        logger::info("Philosophy enchantment bonus: {}.", potionBonus);
        clampedXP *= 1 + (potionBonus / 100.0f);
        customSkills->AdvanceSkill("Philosophy", clampedXP);
        logger::info("Advancing Philosophy Skill +{}.", clampedXP);
        UpdateAvidReader(true);
        UpdateErudite(true);
        LuckyHand();
    }
}

bool Philosophy::ReadBookReference::thunk(RE::TESObjectBOOK* a1, RE::PlayerCharacter* a2) {
    logger::trace("Hook: Read from world");
    PhilosophyProcessBookXP(a1);
    return func(a1, a2);
}

bool Philosophy::ReadBookInventory::thunk(RE::TESObjectBOOK* a1, RE::PlayerCharacter* a2) {
    logger::trace("Hook: Read from Inventory");
    PhilosophyProcessBookXP(a1);
    return func(a1, a2);
}

bool Philosophy::ReadBookContainer::thunk(RE::TESObjectBOOK* a1, RE::PlayerCharacter* a2) {
    logger::trace("Hook: Read from container");
    PhilosophyProcessBookXP(a1);
    return func(a1, a2);
}

void Philosophy::ReadBookInventory::Install() {
    REL::Relocation<std::uintptr_t> target{REL::ID(51870), 0x1A7};
    stl::write_thunk_call<ReadBookInventory>(target.address());
}

void Philosophy::ReadBookReference::Install() {
    REL::Relocation<std::uintptr_t> target{REL::ID(51053), 0x231};
    stl::write_thunk_call<ReadBookReference>(target.address());
}

void Philosophy::ReadBookContainer::Install() {
    REL::Relocation<std::uintptr_t> target{REL::ID(51149), 0x18E};
    stl::write_thunk_call<ReadBookContainer>(target.address());
}

void Philosophy::UpdateAvidReader(bool fromEvent) {
    if (auto player = RE::PlayerCharacter::GetSingleton()) {
        uint32_t books = QueryStat("Books Read");
        // fix for stats not being updated fast enough after triggering event
        if (fromEvent) {
            books += 1;
        }
        RE::BGSPerk* readerPerk = RE::TESForm::LookupByEditorID<RE::BGSPerk>("FNS_PHL_AvidReader");
        auto readerSpell = RE::TESForm::LookupByEditorID<RE::SpellItem>("FNS_PHL_AvidReaderSpell");
        if (player->HasPerk(readerPerk)) {
            player->RemoveSpell(readerSpell);
            if (auto caster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant); caster) {
                if (readerSpell) {
                    int magnitude = floor(books / 5);
                    logger::info("[Avid Reader] Casting updated Avid Reader spell with magnitude {}", magnitude);
                    caster->CastSpellImmediate(readerSpell, false, player, 1.0f, false, magnitude, player);
                }
            }
        }
    }
}

void Philosophy::LuckyHand() {
    auto player = RE::PlayerCharacter::GetSingleton();
    RE::BGSPerk* luckyHandPerk = RE::TESForm::LookupByEditorID<RE::BGSPerk>("FNS_PHL_LuckyHand");
    if (!player->HasPerk(luckyHandPerk)) {
        return;
    }
    int random = RandomInt(0, 10);
    logger::info("[Lucky Hand] Rolling: {}", random);
    if (random < 10) {
        logger::info("[Lucky Hand] Failed dice roll!");
        return;
    }
    logger::info("[Lucky Hand] Dice roll successful!");
    static auto scrollsLeveledList = RE::TESForm::LookupByEditorID<RE::TESLevItem>("LItemScrolls");
    RE::ScrollItem* scroll = ProcessScrollsList(scrollsLeveledList);
    if (auto boundObj = scroll->As<RE::TESBoundObject>()) {
        logger::info("[Lucky Hand] Giving {} to player.", scroll->GetName());
        player->AddObjectToContainer(boundObj, nullptr, 1, nullptr);
        static auto msgRefBegin = RE::TESForm::LookupByEditorID<RE::BGSMessage>("FNS_LuckyHandMessageBegin");
        static auto msgRefEnd = RE::TESForm::LookupByEditorID<RE::BGSMessage>("FNS_LuckyHandMessageEnd");
        RE::BSString msgBegin;
        RE::BSString msgEnd;
        std::string msgFull;
        msgRefBegin->GetDescription(msgBegin, msgRefBegin);
        msgRefEnd->GetDescription(msgEnd, msgRefEnd);
        std::string msgStrBegin = msgBegin.c_str();
        std::string msgStrEnd = msgEnd.c_str();
        msgFull.append(msgStrBegin).append(scroll->GetName()).append(msgStrEnd);
        RE::DebugNotification(msgFull.c_str());
    }
}

RE::ScrollItem* Philosophy::ProcessScrollsList(RE::TESLevItem* scrollsLeveledList) {
    RE::ScrollItem* defaultScroll = RE::TESForm::LookupByEditorID<RE::ScrollItem>("CourageScroll");
    int random = RandomInt(0, scrollsLeveledList->numEntries - 1);
    auto itemform = scrollsLeveledList->entries[random].form;
    if (itemform) {
        if (itemform->formType == RE::FormType::Scroll) {
            auto scrollitem = itemform->As<RE::ScrollItem>();
            logger::info("[Lucky Hand] Scroll: {}", scrollitem->GetName());
            return scrollitem;
        } else if (itemform->formType == RE::FormType::LeveledItem) {
            logger::info("[Lucky Hand] Processing scrolls sublist");
            auto sublist = itemform->As<RE::TESLevItem>();
            return ProcessScrollsList(sublist);
        }
    }
    logger::info("[Lucky Hand] No scrolls found in list, returning default scroll.");
    return defaultScroll;
}

void Philosophy::ApplyMonkBonus() {
    auto* player = RE::PlayerCharacter::GetSingleton();
    auto* activeEffects = player->AsMagicTarget()->GetActiveEffectList();
    if (activeEffects) {
        for (auto activeEffect : *activeEffects) {
            if (!activeEffect || !activeEffect->effect || !activeEffect->effect->baseEffect) continue;

            static auto blessingKeyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("MagicBlessing");
            if (activeEffect->effect->baseEffect->HasKeyword(blessingKeyword)) {
                /*if (activeEffect->elapsedSeconds > 1.f) {
                    logger::trace("[Monk] Blessing effect not recently applied, skipping.");
                    return;
                }*/
                
                logger::trace("[Monk] Blessing effect detected: {}, duration {}, elapsedSeconds {}, magnitude {}",
                             activeEffect->effect->baseEffect->GetName(), activeEffect->duration,
                             activeEffect->elapsedSeconds,
                             activeEffect->magnitude);
                activeEffect->duration = activeEffect->effect->effectItem.duration * 2;
                activeEffect->magnitude = activeEffect->effect->effectItem.magnitude * 1.5f;
                logger::trace(
                    "[Monk] Applying Monk multipliers: duration (x2) {}, elapsedSeconds {}, magnitude (x1.5) {}",
                    activeEffect->duration, activeEffect->elapsedSeconds, activeEffect->magnitude);
            }
        }
    }
}

void Philosophy::UpdateCultist(bool fromEvent) {
    if (auto player = RE::PlayerCharacter::GetSingleton()) {
        uint32_t daedricQuests = QueryStat("Daedric Quests Completed");
        // fix for stats not being updated fast enough after triggering event
        if (fromEvent) {
            daedricQuests += 1;
        }
        RE::BGSPerk* cultistPerk = RE::TESForm::LookupByEditorID<RE::BGSPerk>("FNS_PHL_Cultist");
        auto cultistSpell = RE::TESForm::LookupByEditorID<RE::SpellItem>("FNS_PHL_CultistSpell");
        if (player->HasPerk(cultistPerk)) {
            player->RemoveSpell(cultistSpell);
            if (auto caster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant); caster) {
                if (cultistSpell) {
                    int magnitude = daedricQuests * 3;
                    logger::info("[Cultist] Casting updated Cultist spell with magnitude {}", magnitude);
                    caster->CastSpellImmediate(cultistSpell, false, player, 1.0f, false, magnitude, player);
                }
            }
        }
    }
}

void Philosophy::UpdateHermit() {
    logger::info("[Hermit] Updating Hermit spell.");
    auto* player = RE::PlayerCharacter::GetSingleton();
    RE::BGSPerk* hermitPerk = RE::TESForm::LookupByEditorID<RE::BGSPerk>("FNS_PHL_Hermit");
    auto hermitSpell = RE::TESForm::LookupByEditorID<RE::SpellItem>("FNS_PHL_HermitSpell");
    if (player->HasPerk(hermitPerk)) {
        if (auto caster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant); caster) {
            if (hermitSpell) {
                int magnitude = 5 * hermitDaysCounter;
                logger::info("[Hermit] Casting updated Hermit spell with magnitude {}", magnitude);
                caster->CastSpellImmediate(hermitSpell, false, player, 1.0f, false, magnitude, player);
            }
        }
    }
}

void Philosophy::ResetHermitCounter() {
    logger::info("[Hermit] Player entered dialogue, removing Hermit bonus.");
    auto* player = RE::PlayerCharacter::GetSingleton();
    auto hermitSpell = RE::TESForm::LookupByEditorID<RE::SpellItem>("FNS_PHL_HermitSpell");
    static auto hermitDaysPassed = RE::TESForm::LookupByEditorID<RE::TESGlobal>("HermitDaysPassed");
    static auto gameDaysPassed = RE::TESForm::LookupByEditorID<RE::TESGlobal>("GameDaysPassed");
    hermitDaysPassed->value = gameDaysPassed->value;
    hermitDaysCounter = 0;
    if (auto caster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant); caster) {
        if (hermitSpell) {
            caster->CastSpellImmediate(hermitSpell, false, player, 1.0f, false, 0.f, player);
        }
    }
}

void Philosophy::EvaluateHermit() {
    static auto hermitDaysPassed = RE::TESForm::LookupByEditorID<RE::TESGlobal>("HermitDaysPassed");
    static auto gameDaysPassed = RE::TESForm::LookupByEditorID<RE::TESGlobal>("GameDaysPassed");
    auto gameDays = gameDaysPassed->value;
    auto hermitDays = hermitDaysPassed->value;
    if (floor(gameDays - hermitDays) > hermitDaysCounter) {
        hermitDaysCounter = floor(gameDays - hermitDays);
        UpdateHermit();
    }
}

void Philosophy::UpdateErudite(bool fromEvent) {
    auto player = RE::PlayerCharacter::GetSingleton();
    uint32_t books = QueryStat("Books Read");
    // fix for stats not being updated fast enough after triggering event
    if (fromEvent) {
        books += 1;
    }
    int bonusPoints = floor(books / 20);
    RE::BGSPerk* eruditePerk = RE::TESForm::LookupByEditorID<RE::BGSPerk>("FNS_PHL_Erudite");
    if (!player->HasPerk(eruditePerk)) {
        return;
    }

    auto eruditeGlobal = RE::TESForm::LookupByEditorID<RE::TESGlobal>("EruditePointsGained");
    if (bonusPoints <= eruditeGlobal->value) {
        return;
    }
    int pointsToGive = bonusPoints - eruditeGlobal->value;
    eruditeGlobal->value = bonusPoints;
    player->GetPlayerRuntimeData().perkCount += pointsToGive;
    logger::info("[Erudite] Player read {} books, +{} perk points (total received {}).", books, pointsToGive, bonusPoints);
}