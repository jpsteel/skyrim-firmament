#include "Horseman.h"
#include "Utility.h"
#include "Skills.h"
#include "Lookup.h"
#include "RE\Offsets.h"
#include "RE\B\BGSSaveLoadGame.h"

float baseSpeechEasy;
float baseSpeechAverage;
float baseSpeechHard;
float baseSpeechVeryHard;

RE::BSEventNotifyControl Horseman::EventProcessor::ProcessEvent(const RE::MenuOpenCloseEvent* event,
                                                      RE::BSTEventSource<RE::MenuOpenCloseEvent>*) {
    if (!event) {
        return RE::BSEventNotifyControl::kContinue;
    }

    static const auto nobilityPerk = RE::TESForm::LookupByEditorID<RE::BGSPerk>("FNS_HRS_Nobility");
    static auto easy = RE::TESForm::LookupByEditorID<RE::TESGlobal>("SpeechEasy");
    static auto average = RE::TESForm::LookupByEditorID<RE::TESGlobal>("SpeechAverage");
    static auto hard = RE::TESForm::LookupByEditorID<RE::TESGlobal>("SpeechHard");
    static auto veryHard = RE::TESForm::LookupByEditorID<RE::TESGlobal>("SpeechVeryHard");

    if (!easy || !average || !hard || !veryHard) {
        logger::warn("[Nobility] One or more persuasion globals failed to load.");
        return RE::BSEventNotifyControl::kContinue;
    }
    auto* player = RE::PlayerCharacter::GetSingleton();
    const bool onHorseWithPerk = player->IsOnMount() && player->HasPerk(nobilityPerk);

    if (event->opening && event->menuName == RE::DialogueMenu::MENU_NAME) {
        if (onHorseWithPerk) {
            baseSpeechEasy = easy->value;
            baseSpeechAverage = average->value;
            baseSpeechHard = hard->value;
            baseSpeechVeryHard = veryHard->value;
            logger::info("[Nobility] Saved persuasion values:\neasy {}, average {}, hard {}, very hard {}", easy->value,
                         average->value, hard->value, veryHard->value);
            float newSpeechEasy = easy->value * 0.5;
            float newSpeechAverage = average->value * 0.5;
            float newSpeechHard = hard->value * 0.5;
            float newSpeechVeryHard = veryHard->value * 0.5;
            logger::info("[Nobility] Setting new persuasion values:\neasy {}, average {}, hard {}, very hard {}",
                         newSpeechEasy,
                         newSpeechAverage, newSpeechHard, newSpeechVeryHard);
            easy->value = newSpeechEasy;
            average->value = newSpeechAverage;
            hard->value = newSpeechHard;
            veryHard->value = newSpeechVeryHard;
        }
    } else if (!event->opening && event->menuName == RE::DialogueMenu::MENU_NAME) {
        if (onHorseWithPerk) {
            easy->value = baseSpeechEasy;
            average->value = baseSpeechAverage;
            hard->value = baseSpeechHard;
            veryHard->value = baseSpeechVeryHard;
            logger::info("[Nobility] Restoring original persuasion values:\neasy {}, average {}, hard {}, very hard {}",
                         baseSpeechEasy, baseSpeechAverage, baseSpeechHard, baseSpeechVeryHard);
        }
    } else if (!event->opening && event->menuName == RE::StatsMenu::MENU_NAME) {
        UpdateJouster();
    }

    return RE::BSEventNotifyControl::kContinue;
}

void Horseman::InstallHooks() {
    auto& trampoline = SKSE::GetTrampoline();

    REL::Relocation<uintptr_t> processhit_hook{RELOCATION_ID(37673, 38627)};
    _ProcessHit = trampoline.write_call<5>(processhit_hook.address() + REL::Relocate(0x3C0, 0x4A8), ProcessHit);

    MoveSpeedPatch();
    SpeedMultModifiedPatch();

    REL::Relocation<uintptr_t> AnimEventVtbl_NPC{RE::VTABLE_Character[2]};
    REL::Relocation<uintptr_t> AnimEventVtbl_PC{RE::VTABLE_PlayerCharacter[2]};

    _ProcessEvent_NPC = AnimEventVtbl_NPC.write_vfunc(0x1, ProcessEvent_NPC);
    _ProcessEvent_PC = AnimEventVtbl_PC.write_vfunc(0x1, ProcessEvent_PC);

    logger::info("Horseman hooks successfully installed.");
}

void Horseman::ProcessHit(RE::Actor* victim, RE::HitData& hitData) {
    if (!victim || victim->IsDead()) {
        _ProcessHit(victim, hitData);
        return;
    }

    const auto aggressor = hitData.aggressor.get();

    if (aggressor && aggressor->IsPlayerRef()) {
        if (!aggressor->IsOnMount()) {
            _ProcessHit(victim, hitData);
            return;
        }

        logger::trace("Player dealt {} damage on horseback.", hitData.totalDamage);
        if (const auto customSkills = GetCustomSkillsInterface()) {
            customSkills->AdvanceSkill("Horseman", hitData.totalDamage);
            logger::info("Advancing Horseman Skill + {}.", hitData.totalDamage);
        }
    }

    _ProcessHit(victim, hitData);
}

void Horseman::MoveSpeedPatch() {
    auto hook = REL::Relocation<std::uintptr_t>(RE::Offset::Actor::ComputeMovementType, 0x51);
    REL::make_pattern<"E8">().match_or_fail(hook.address());

    // TRAMPOLINE: 14
    auto& trampoline = SKSE::GetTrampoline();
    _GetScale = trampoline.write_call<5>(hook.address(), &GetSpeedMult);
}

void Horseman::SpeedMultModifiedPatch() {
    auto callbacks = REL::Relocation<std::uintptr_t>(RE::Offset::Actor::ActorValueModifiedCallbacks);

    _SpeedMultModifiedCallback =
        callbacks.write_vfunc(static_cast<std::size_t>(RE::ActorValue::kSpeedMult), &SpeedMultModifiedCallback);
}

static void ForceUpdateCachedMovementType(RE::Actor* a_actor) {
    using func_t = decltype(&ForceUpdateCachedMovementType);
    REL::Relocation<func_t> func{RE::Offset::Actor::ForceUpdateCachedMovementType};
    return func(a_actor);
}

static void SpeedAffectingValueModified(RE::Actor* a_actor) {
    const auto saveLoadGame = RE::BGSSaveLoadGame::GetSingleton();
    if (!saveLoadGame->GetSaveGameLoading()) {
        ForceUpdateCachedMovementType(a_actor);
    }
}

void Horseman::SpeedMultModifiedCallback(RE::Actor* a_actor, RE::ActorValue a_actorValue, float a_oldValue,
                                         float a_delta,
                               RE::TESObjectREFR* a_cause) {
    SpeedAffectingValueModified(a_actor);

    if (_SpeedMultModifiedCallback.address()) {
        _SpeedMultModifiedCallback(a_actor, a_actorValue, a_oldValue, a_delta, a_cause);
    }
}

float Horseman::GetSpeedMult(const RE::Actor* a_actor) {
    float scale = _GetScale(a_actor);
    if (IsPlayersMount(a_actor)) {
        float enchantBonus = RE::PlayerCharacter::GetSingleton()->AsActorValueOwner()->GetActorValue(
            LookupActorValueByName("HorsemanEnchantments"));
        logger::trace("Horseman enchantment/perk bonus: {}", enchantBonus);
        float potionBonus = RE::PlayerCharacter::GetSingleton()->AsActorValueOwner()->GetActorValue(
            LookupActorValueByName("HorsemanPotions"));
        logger::trace("Horseman potion bonus: {}", potionBonus);

        const float skillMult = 1.0f + (enchantBonus / 100.0f) + (potionBonus / 100.0f);
        logger::trace("Calculating {} speed scale: {}", a_actor->GetName(), skillMult);
        scale *= skillMult;
    }
    return scale;
}

void Horseman::UpdateJouster() {
    auto* player = RE::PlayerCharacter::GetSingleton();
    static const auto jousterPerk = RE::TESForm::LookupByEditorID<RE::BGSPerk>("FNS_HRS_Jouster");
    static const auto chargeSpell = RE::TESForm::LookupByEditorID<RE::SpellItem>("FNS_HRS_ChargeCloakAbility");
    if (player->HasPerk(jousterPerk)) {
        RE::NiPointer<RE::Actor> playerMount;
        if (player->IsOnMount()) {
            logger::info("[Jouster] Adding Jouster spell to player's mount");
            player->GetMount(playerMount);
            playerMount.get()->AddSpell(chargeSpell);
        }
    }
}

void Horseman::ProcessEvent(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_sink, RE::BSAnimationGraphEvent* a_event,
                            RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource) {
    if (!a_event || !a_event->holder) {
        return;
    }

    auto* actor = a_event->holder->As<RE::Actor>();
    if (!actor) return;

    auto player = RE::PlayerCharacter::GetSingleton();

    static const auto knightPerk = RE::TESForm::LookupByEditorID<RE::BGSPerk>("FNS_HRS_Knight");
    static const auto jousterPerk = RE::TESForm::LookupByEditorID<RE::BGSPerk>("FNS_HRS_Jouster");
    static const auto fearSpell = RE::TESForm::LookupByEditorID<RE::SpellItem>("FNS_HRS_HorseFearSpell1");
    static const auto chargeSpell = RE::TESForm::LookupByEditorID<RE::SpellItem>("FNS_HRS_ChargeCloakAbility");

    if (IsPlayersMount(actor)) {
        if (player && player->IsOnMount() && player->HasPerk(knightPerk)) {
            if (a_event->tag == "idleRearUp") {
                if (auto caster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant); caster) {
                    logger::info("[Knight] Casting fear spell from player");
                    caster->CastSpellImmediate(fearSpell, false, player, 1.0f, false, 0.0f, player);
                }
            }
        }
    }

    if (actor == player && player->HasPerk(jousterPerk)) {
        RE::NiPointer<RE::Actor> playerMount;
        if (a_event->tag == "tailHorseMount") {
            logger::info("[Jouster] Adding Jouster spell to player's mount");
            player->GetMount(playerMount);
            playerMount.get()->AddSpell(chargeSpell);
        } else if (a_event->tag == "tailHorseDismount") {
            logger::info("[Jouster] Removing Jouster spell from player's mount");
            player->GetMount(playerMount);
            playerMount.get()->RemoveSpell(chargeSpell);
        }
    }
}

RE::BSEventNotifyControl Horseman::ProcessEvent_NPC(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_sink,
                                               RE::BSAnimationGraphEvent* a_event,
                                               RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource) {
    ProcessEvent(a_sink, a_event, a_eventSource);
    return _ProcessEvent_NPC(a_sink, a_event, a_eventSource);
}

RE::BSEventNotifyControl Horseman::ProcessEvent_PC(
    RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_sink,
                                              RE::BSAnimationGraphEvent* a_event,
                                              RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource) {
    ProcessEvent(a_sink, a_event, a_eventSource);
    return _ProcessEvent_PC(a_sink, a_event, a_eventSource);
}