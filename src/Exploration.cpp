#include "Exploration.h"
#include "Utility.h"
#include "editorID.hpp"
#include "Skills.h"

bool playerIsMining = false;
bool stopAddingOre = false;
bool Exploration::appliedDungeonDelver = false;
bool Exploration::initializedCachedValues = false;

RE::BSEventNotifyControl Exploration::EventProcessor::ProcessEvent(const RE::LocationDiscovery::Event* event,
                                                      RE::BSTEventSource<RE::LocationDiscovery::Event>* eventSource) {

    if (event) {
        logger::trace("Location discovered: {}", event->mapMarkerData->locationName.GetFullName());
        if (const auto customSkills = GetCustomSkillsInterface()) {
            customSkills->AdvanceSkill("Exploration", 50);
            logger::trace("Advancing Exploration Skill.");
        }
        UpdateCartographer(true);
    }

    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl Exploration::EventProcessor::ProcessEvent(
    const RE::TESActivateEvent* event, RE::BSTEventSource<RE::TESActivateEvent>* eventSource) {

    if (!event) {
        return RE::BSEventNotifyControl::kContinue;
    }

    auto player = RE::PlayerCharacter::GetSingleton();
    
    static const auto pilgrimPerk = RE::TESForm::LookupByEditorID<RE::BGSPerk>("FNS_EXP_Pilgrim");
    if (event->objectActivated && event->actionRef) {
        if (!event->actionRef.get()->IsPlayerRef()) {
            return RE::BSEventNotifyControl::kContinue;
        }
        auto pilgrimSpell = RE::TESForm::LookupByEditorID<RE::SpellItem>("FNS_EXP_PilgrimClearSkiesSpell");
        auto object = event->objectActivated.get()->GetBaseObject();
        auto editorID = clib_util::editorID::get_editorID(object);
        if (player->HasPerk(pilgrimPerk)) {
            if (editorID.contains("Shrine") && !player->GetParentCell()->IsInteriorCell()) {
                PilgrimClearSkies();
            }
        }
        if (editorID.contains("Mining")) {
            logger::info("[Diamond Pickaxe] Player started mining");
            playerIsMining = true;
        }
    }
    
    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl Exploration::EventProcessor::ProcessEvent(const SKSE::ModCallbackEvent* event,
                                                      RE::BSTEventSource<SKSE::ModCallbackEvent>* eventSource) {
    if (!event) {
        return RE::BSEventNotifyControl::kContinue;
    }

    auto player = RE::PlayerCharacter::GetSingleton();
    static const auto pilgrimPerk = RE::TESForm::LookupByEditorID<RE::BGSPerk>("FNS_EXP_Pilgrim");
    if (event->eventName == "FNS_EXP_PilgrimEvent" && !player->GetParentCell()->IsInteriorCell()) {
        if (player->HasPerk(pilgrimPerk)) {
            PilgrimClearSkies();
        }
    }
    //to account for Survival's message box popup
    else if (event->eventName == "FNS_EXP_WayfarerEvent") {
        DiscoverRandomLocation();
    }

    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl Exploration::EventProcessor::ProcessEvent(const RE::TESFastTravelEndEvent* event,
                                                      RE::BSTEventSource<RE::TESFastTravelEndEvent>* eventSource) {
    if (!event) {
        return RE::BSEventNotifyControl::kContinue;
    }

    if (event) {
        logger::info("[Lay of the Land] Restarting \"Lay of the Land\" cooldown.");
        static auto perkDaysPassed = RE::TESForm::LookupByEditorID<RE::TESGlobal>("LayOfTheLandDaysPassed");
        static auto gameDaysPassed = RE::TESForm::LookupByEditorID<RE::TESGlobal>("GameDaysPassed");
        perkDaysPassed->value = gameDaysPassed->value + 1.f;
    }

    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl Exploration::EventProcessor::ProcessEvent(
    const RE::MenuOpenCloseEvent* event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* eventSource) {
    if (!event) {
        return RE::BSEventNotifyControl::kContinue;
    }

    static auto survivalEnabled = RE::TESForm::LookupByEditorID<RE::TESGlobal>("Survival_ModeEnabled");
    if (event->menuName == RE::MapMenu::MENU_NAME) {
        if (event->opening) {
            originalSurvivalEnabled = survivalEnabled->value;
            static const auto layOfTheLandPerk = RE::TESForm::LookupByEditorID<RE::BGSPerk>("FNS_EXP_LayOfTheLand");
            auto player = RE::PlayerCharacter::GetSingleton();
            if (!player->HasPerk(layOfTheLandPerk)) {
                return RE::BSEventNotifyControl::kContinue;
            }
            static auto perkDaysPassed = RE::TESForm::LookupByEditorID<RE::TESGlobal>("LayOfTheLandDaysPassed");
            static auto gameDaysPassed = RE::TESForm::LookupByEditorID<RE::TESGlobal>("GameDaysPassed");
            if (perkDaysPassed->value - gameDaysPassed->value < 0.f) {
                survivalEnabled->value = 0.f;
                static auto msgRef = RE::TESForm::LookupByEditorID<RE::BGSMessage>("FNS_LayOfTheLandMessage");
                RE::BSString msg;
                msgRef->GetDescription(msg, msgRef);
                RE::DebugNotification(msg.c_str());
            }
        } else if (!event->opening) {
            survivalEnabled->value = originalSurvivalEnabled;
        }
    } else if (!event->opening && event->menuName == RE::LoadingMenu::MENU_NAME) {
        UpdateDungeonDelver();
    } else if (!event->opening && event->menuName == RE::StatsMenu::MENU_NAME) {
        UpdateDungeonDelver();
        UpdateCartographer(false);
        auto player = RE::PlayerCharacter::GetSingleton();
        logger::info("Player warmth rating: {}", GetWarmthRating(player));
    }

    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl Exploration::EventProcessor::ProcessEvent(
    const RE::TESContainerChangedEvent* event, RE::BSTEventSource<RE::TESContainerChangedEvent>* eventSource) {
    if (!event) {
        return RE::BSEventNotifyControl::kContinue;
    }

    auto player = RE::PlayerCharacter::GetSingleton();
    RE::BGSPerk* miningPerk = RE::TESForm::LookupByEditorID<RE::BGSPerk>("FNS_EXP_DiamondPickaxe");
    if (event->newContainer == player->GetFormID()) {
        if (!player->HasPerk(miningPerk) || stopAddingOre) {
            return RE::BSEventNotifyControl::kContinue;
        }
        auto form = RE::TESForm::LookupByID(event->baseObj);
        auto editorID = clib_util::editorID::get_editorID(form);
        logger::trace("[Diamond Pickaxe] Player received item: {} ({})", form->GetName(), editorID);
        if (playerIsMining) {
            if (auto boundObj = form->As<RE::TESBoundObject>()) {
                logger::trace("[Diamond Pickaxe] Adding a second one!");
                stopAddingOre = true;
                player->AddObjectToContainer(boundObj, nullptr, 1, nullptr);
                stopAddingOre = false;
            }
        }
    }

    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl Exploration::EventProcessor::ProcessEvent(const RE::TESHitEvent* event,
                                                                   RE::BSTEventSource<RE::TESHitEvent>* eventSource) {
    if (!event || !event->target) {
        return RE::BSEventNotifyControl::kContinue;
    }

    auto player = RE::PlayerCharacter::GetSingleton();
    if (event->cause.get() != player) {
        logger::trace("[Hunter] Not hit by player");
        return RE::BSEventNotifyControl::kContinue;
    }

    auto* target = event->target.get();
    auto* targetActor = target->As<RE::Actor>();
    if (!targetActor || !targetActor->GetRace()->HasKeywordString("ActorTypeAnimal")) {
        logger::trace("[Hunter] Target not an ActorTypeAnimal");
        return RE::BSEventNotifyControl::kContinue;
    }

    static auto hunterPerk = RE::TESForm::LookupByEditorID<RE::BGSPerk>("FNS_EXP_Hunter");
    if (!player->HasPerk(hunterPerk)) {
        logger::trace("[Hunter] Player doesn't have Hunter perk");
        return RE::BSEventNotifyControl::kContinue;
    }

    static auto slowEffect = RE::TESForm::LookupByEditorID<RE::EffectSetting>("FNS_EXP_HunterSlowEffect");
    if (HasEffect(targetActor, slowEffect)) {
        logger::trace("[Hunter] Target already slowed.");
        return RE::BSEventNotifyControl::kContinue;
    }

    static auto slowSpell = RE::TESForm::LookupByEditorID<RE::SpellItem>("FNS_EXP_HunterSlowSpell");
    if (slowSpell) {
        logger::info("[Hunter] Applying Hunter slow effect to {}", targetActor->GetName());
        if (auto caster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant); caster) {
            caster->CastSpellImmediate(slowSpell, false, targetActor, 1.0f, false, 20.0f, player);
        }
    }

    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl Exploration::EventProcessor::ProcessEvent(const RE::TESCombatEvent* event, RE::BSTEventSource<RE::TESCombatEvent>* eventSource) {
    if (!event || !event->actor || !event->targetActor) {
        return RE::BSEventNotifyControl::kContinue;
    }

    auto* player = RE::PlayerCharacter::GetSingleton();
    auto* targetActor = event->targetActor.get()->As<RE::Actor>();
    if (!targetActor->IsPlayerRef() && !IsPlayersMount(targetActor)) {
        logger::trace("[Wanderer's Peace] Player not involved in combat.");
        return RE::BSEventNotifyControl::kContinue;
    }

    static auto npcKeyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("ActorTypeNPC");
    static auto undeadKeyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("ActorTypeUndead");
    if (!event->actor->HasKeyword(npcKeyword) || event->actor->HasKeyword(undeadKeyword)) {
        logger::trace("[Wanderer's Peace] Actor not an NPC (or undead).");
        return RE::BSEventNotifyControl::kContinue;
    }

    static auto wandererPerk = RE::TESForm::LookupByEditorID<RE::BGSPerk>("FNS_EXP_WanderersPeace");
    static auto calmSpell = RE::TESForm::LookupByEditorID<RE::SpellItem>("FNS_EXP_WanderersPeaceSpell");
    if (!player->HasPerk(wandererPerk)) {
        logger::trace("[Wanderer's Peace] Player hasn't unlocked Wanderer's Peace.");
        return RE::BSEventNotifyControl::kContinue;
    }

    if (player->AsActorState()->GetWeaponState() == RE::WEAPON_STATE::kDrawing ||
        player->AsActorState()->GetWeaponState() == RE::WEAPON_STATE::kDrawn) {
        logger::trace("[Wanderer's Peace] Player has unsheathed weapons.");
        return RE::BSEventNotifyControl::kContinue;
    }

    static auto peaceDaysPassed = RE::TESForm::LookupByEditorID<RE::TESGlobal>("WanderersPeaceDaysPassed");
    static auto gameDaysPassed = RE::TESForm::LookupByEditorID<RE::TESGlobal>("GameDaysPassed");
    if (peaceDaysPassed->value - gameDaysPassed->value > 0.f) {
        logger::info("[Wanderer's Peace] Less than one hour has passed since last failed roll.");
        return RE::BSEventNotifyControl::kContinue;
    }

    int random = RandomInt(0, 100);
    logger::info("[Wanderer's Peace] Random num: {} (<=70?)", random);
    if (random > 70) {
        logger::info("[Wanderer's Peace] Failed dice roll!");
        static auto msgRef = RE::TESForm::LookupByEditorID<RE::BGSMessage>("FNS_WanderersPeaceMessageFail");
        RE::BSString msg;
        msgRef->GetDescription(msg, msgRef);
        RE::DebugNotification(msg.c_str());
        peaceDaysPassed->value = gameDaysPassed->value + 0.05;
        logger::info("[Wanderer's Peace] Restarting \"Wanderer’s Peace\" cooldown.");
        return RE::BSEventNotifyControl::kContinue;
    }
    logger::info("[Wanderer's Peace] Successful dice roll!");

    if (auto caster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant); caster) {
        logger::info("[Wanderer's Peace] Casting Wanderer’s Peace spell.");
        caster->CastSpellImmediate(calmSpell, false, player, 1.0f, false, 0.0f, player);
        static auto msgRef = RE::TESForm::LookupByEditorID<RE::BGSMessage>("FNS_WanderersPeaceMessageSuccess");
        RE::BSString msg;
        msgRef->GetDescription(msg, msgRef);
        RE::DebugNotification(msg.c_str());
    }

    return RE::BSEventNotifyControl::kContinue;
}

void Exploration::InstallHooks() {
    REL::Relocation<uintptr_t> AnimEventVtbl_PC{RE::VTABLE_PlayerCharacter[2]};
    _ProcessEvent_PC = AnimEventVtbl_PC.write_vfunc(0x1, ProcessEvent_PC);

    logger::info("Exploration hooks successfully installed.");
}

void Exploration::UpdateCartographer(bool fromEvent) {
    if (auto player = RE::PlayerCharacter::GetSingleton()) {
        uint32_t locs = QueryStat("Locations Discovered");
        // fix for stats not being updated fast enough after loc discovery event
        if (fromEvent) {
            locs += 1;
        }
        RE::BGSPerk* cartographerPerk = RE::TESForm::LookupByEditorID<RE::BGSPerk>("FNS_EXP_Cartographer");
        auto cartographerSpell = RE::TESForm::LookupByEditorID<RE::SpellItem>("FNS_EXP_CartographerSpell");
        auto cartographerEffect = RE::TESForm::LookupByEditorID<RE::EffectSetting>("FNS_EXP_CartographerCarryWeight");
        bool needUpdate = true;
        if (player->HasPerk(cartographerPerk)) {
            if (player->AsMagicTarget()->HasMagicEffect(cartographerEffect)) {
                logger::info("[Cartographer] Player already has MGEF.");
                auto* activeEffects = player->AsMagicTarget()->GetActiveEffectList();
                if (activeEffects) {
                    for (auto activeEffect : *activeEffects) {
                        if (!activeEffect || !activeEffect->effect || !activeEffect->effect->baseEffect) continue;

                        if (activeEffect->effect->baseEffect == cartographerEffect) {
                            logger::info(
                                "[Cartographer] Found magic effect in player's active effects list, magnitude {}.",
                                activeEffect->magnitude);

                            if (activeEffect->magnitude == locs) {
                                needUpdate = false;
                            }
                        }
                    }
                }
            }
        }
        if (needUpdate) {
            if(auto caster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant); caster) {
                if (cartographerSpell) {
                    logger::info("[Cartographer] Casting cartographer spell with magnitude {}", locs);
                    caster->CastSpellImmediate(cartographerSpell, true, player, 1.0f, false, locs, player);
                }
            }
        }
    }
}

void Exploration::PilgrimClearSkies() {
    auto pilgrimSpell = RE::TESForm::LookupByEditorID<RE::SpellItem>("FNS_EXP_PilgrimClearSkiesSpell");
    logger::info("[Pilgrim] Player prayed at a shrine while outside.");
    int random = RandomInt(0, 100);
    logger::trace("[Pilgrim] Random num: {}, Player's Exploration level: {}", random, Data::Exploration::GetLevel());
    if (random <= Data::Exploration::GetLevel()) {
        logger::info("[Pilgrim] Successful dice roll!");
        auto player = RE::PlayerCharacter::GetSingleton();
        if (auto caster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant); caster) {
            if (pilgrimSpell) {
                static auto msgRef = RE::TESForm::LookupByEditorID<RE::BGSMessage>("FNS_PilgrimMessage");
                RE::BSString msg;
                msgRef->GetDescription(msg, msgRef);
                RE::DebugNotification(msg.c_str());
                caster->CastSpellImmediate(pilgrimSpell, false, player, 1.0f, false, 0.0f, player);
            }
        }
    } else {
        logger::info("[Pilgrim] Failed dice roll!");
    }
}

void Exploration::DiscoverRandomLocation() {
    RE::BSTArray<RE::ObjectRefHandle> undiscoveredLocs = GetUndiscoveredMapMarkers();

    if (undiscoveredLocs.size() == 0) {
        static auto msgRef = RE::TESForm::LookupByEditorID<RE::BGSMessage>("FNS_WayfarerMessageAllDiscovered");
        RE::BSString msg;
        msgRef->GetDescription(msg, msgRef);
        std::string msgStr = msg.c_str();
        RE::DebugNotification(msgStr.c_str());
        return;
    }

    int max = undiscoveredLocs.size() - 1;
    int rand = RandomInt(0, max);

    auto randomLoc = undiscoveredLocs[rand].get().get();
    if (auto* extraMapMarker = randomLoc->extraList.GetByType<RE::ExtraMapMarker>();
        extraMapMarker && extraMapMarker->mapData) {
        auto locName = extraMapMarker->mapData->locationName.GetFullName();
        logger::info("[Wayfarer] Random undiscovered location: {}", locName);
        extraMapMarker->mapData->flags.set(RE::MapMarkerData::Flag::kVisible, RE::MapMarkerData::Flag::kCanTravelTo,
                                           RE::MapMarkerData::Flag::kDiscovered);
        randomLoc->AddChange(RE::TESObjectREFR::ChangeFlags::kGameOnlyExtra);
        static auto msgRef = RE::TESForm::LookupByEditorID<RE::BGSMessage>("FNS_WayfarerMessage");
        RE::BSString msg;
        msgRef->GetDescription(msg, msgRef);
        std::string msgStr = msg.c_str();
        msgStr.append(locName);
        RE::DebugNotification(msgStr.c_str());
    } else {
        logger::warn("[Wayfarer] No valid ExtraMapMarker found on random location.");
    }
}

bool Exploration::IsPlayerInDungeon() { 
    if (auto player = RE::PlayerCharacter::GetSingleton()) {
        auto cell = player->GetParentCell();
        if (!cell || !cell->IsInteriorCell()) {
            return false;
        }
        auto loc = cell->GetLocation();
        if (!loc) {
            return false;
        }
        RE::BGSKeyword* keyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeDungeon");
        std::vector<RE::BGSKeyword*> keywordsArray;
        keywordsArray.push_back(keyword);
        if (loc->HasKeywordInArray(keywordsArray, false)) {
            return true;
        }
    }
    return false;
}

void Exploration::UpdateCamperWellRested() {
    auto player = RE::PlayerCharacter::GetSingleton();
    auto camper01Perk = RE::TESForm::LookupByEditorID<RE::BGSPerk>("FNS_EXP_Camper");
    auto exhaustionNeedValue = RE::TESForm::LookupByEditorID<RE::TESGlobal>("Survival_ExhaustionNeedValue")->value;
    if (player->HasPerk(camper01Perk)) {
        auto rested = RE::TESForm::LookupByEditorID<RE::SpellItem>("Rested");
        auto wellRested = RE::TESForm::LookupByEditorID<RE::SpellItem>("WellRested");
        if (player->HasSpell(wellRested)) {
            logger::trace("[Camper] Well Rested bonus already applied.");
            if (player->HasSpell(rested)) {
                player->RemoveSpell(rested);
                logger::info("[Camper] Removing Rested bonus.");
            }
            return;
        }

        auto SMIExhaustionStage1 = RE::TESForm::LookupByEditorID<RE::TESGlobal>("Survival_ExhaustionStage1Value");
        float threshold = 80.f;
        if (SMIExhaustionStage1) {
            threshold = SMIExhaustionStage1->value;
            logger::trace("[Camper] Found Survival Mode Improved value, setting threshold to {}", threshold);
        } else {
            logger::trace("[Camper] Couldn't find Survival Mode Improved value, setting threshold to default {}",
                         threshold);
        }

        if (exhaustionNeedValue >= threshold) {
            logger::trace("[Camper] Exhaustion value >= {}, won't apply Well Rested.", threshold);
            return;
        }

        player->AddSpell(wellRested);
        logger::info("[Camper] Applying Well Rested bonus to player.");
        static auto msgRef = RE::TESForm::LookupByEditorID<RE::BGSMessage>("WellRestedMessage");
        RE::BSString msg;
        msgRef->GetDescription(msg, msgRef);
        RE::DebugNotification(msg.c_str());
    }
}

void Exploration::UpdateDungeonDelver() {
    auto exhaustionNeedRate = RE::TESForm::LookupByEditorID<RE::TESGlobal>("Survival_ExhaustionNeedRate");
    auto hungerNeedRate = RE::TESForm::LookupByEditorID<RE::TESGlobal>("Survival_HungerNeedRate");
    auto cachedExhaustionNeedRate = RE::TESForm::LookupByEditorID<RE::TESGlobal>("CachedExhaustionNeedRate");
    auto cachedHungerNeedRate = RE::TESForm::LookupByEditorID<RE::TESGlobal>("CachedHungerNeedRate");

    auto player = RE::PlayerCharacter::GetSingleton();
    auto dungeonDelverPerk = RE::TESForm::LookupByEditorID<RE::BGSPerk>("FNS_EXP_DungeonDelver");

    if (!player->HasPerk(dungeonDelverPerk)) {
        return;
    }

    if (IsPlayerInDungeon()) {
        if (!appliedDungeonDelver) {
            exhaustionNeedRate->value = cachedExhaustionNeedRate->value * 0.5f;
            hungerNeedRate->value = cachedHungerNeedRate->value * 0.5f;

            appliedDungeonDelver = true;
            logger::info("[Dungeon Delver] Dungeon Delver bonus applied.");
        }
    } else {
        if (appliedDungeonDelver) {
            exhaustionNeedRate->value = cachedExhaustionNeedRate->value;
            hungerNeedRate->value = cachedHungerNeedRate->value;

            appliedDungeonDelver = false;
            logger::info("[Dungeon Delver] Dungeon Delver bonus removed.");
        }
    }
}

void Exploration::InitCachedValues() {
    if (!initializedCachedValues) {
        auto exhaustionNeedRate = RE::TESForm::LookupByEditorID<RE::TESGlobal>("Survival_ExhaustionNeedRate");
        auto hungerNeedRate = RE::TESForm::LookupByEditorID<RE::TESGlobal>("Survival_HungerNeedRate");
        auto cachedExhaustionNeedRate = RE::TESForm::LookupByEditorID<RE::TESGlobal>("CachedExhaustionNeedRate");
        auto cachedHungerNeedRate = RE::TESForm::LookupByEditorID<RE::TESGlobal>("CachedHungerNeedRate");

        if (cachedExhaustionNeedRate->value == 0) {
            cachedExhaustionNeedRate->value = exhaustionNeedRate->value;
        }
        if (cachedHungerNeedRate->value == 0) {
            cachedHungerNeedRate->value = hungerNeedRate->value;
        }

        initializedCachedValues = true;
    }
}

void Exploration::ProcessEvent(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_sink, RE::BSAnimationGraphEvent* a_event,
                            RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource) {
    if (!a_event || !a_event->holder) {
        return;
    }

    auto* actor = a_event->holder->As<RE::Actor>();
    if (!actor) return;

    auto player = RE::PlayerCharacter::GetSingleton();
    if (actor == player && playerIsMining) {
        if (a_event->tag == "IdleFurnitureExit") {
            logger::info("[Diamond Pickaxe] Player stopped mining");
            playerIsMining = false;
        }
    }
}

RE::BSEventNotifyControl Exploration::ProcessEvent_PC(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_sink,
                                                   RE::BSAnimationGraphEvent* a_event,
                                                   RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource) {
    ProcessEvent(a_sink, a_event, a_eventSource);
    return _ProcessEvent_PC(a_sink, a_event, a_eventSource);
}