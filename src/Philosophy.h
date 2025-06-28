#ifndef PHILOSOPHY_H
#define PHILOSOPHY_H

#include <unordered_set>

namespace Philosophy {
    class EventProcessor : public RE::BSTEventSink<RE::MenuOpenCloseEvent>,
                           public RE::BSTEventSink<RE::TESCombatEvent>,
                           public RE::BSTEventSink<RE::TESMagicEffectApplyEvent>,
                           public RE::BSTEventSink<RE::TESQuestStageEvent> {
    public:
        static EventProcessor* GetSingleton() {
            static EventProcessor instance;
            return &instance;
        }

        RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* event,
                                              RE::BSTEventSource<RE::MenuOpenCloseEvent>* eventSource) override;
        RE::BSEventNotifyControl ProcessEvent(const RE::TESCombatEvent* event,
                                              RE::BSTEventSource<RE::TESCombatEvent>* eventSource) override;
        RE::BSEventNotifyControl ProcessEvent(const RE::TESMagicEffectApplyEvent* event,
                                              RE::BSTEventSource<RE::TESMagicEffectApplyEvent>* eventSource) override;
        RE::BSEventNotifyControl ProcessEvent(const RE::TESQuestStageEvent* event,
                                              RE::BSTEventSource<RE::TESQuestStageEvent>* eventSource) override;
    };

    void InstallHooks();
    void PhilosophyProcessBookXP(RE::TESObjectBOOK* book);

    ////////////credits to SeaSparrow for the following hooks (https://github.com/SeaSparrowOG/SpellLearning)
    // Hook triggers when reading a book from a container, but not the player's inventory.
    struct ReadBookContainer {
        static bool thunk(RE::TESObjectBOOK* a1, RE::PlayerCharacter* a2);

        static void Install();

        static inline REL::Relocation<decltype(thunk)> func;
    };

    // Hook triggers when reading a book from the player's inventory.
    struct ReadBookInventory {
        static bool thunk(RE::TESObjectBOOK* a1, RE::PlayerCharacter* a2);

        static void Install();

        static inline REL::Relocation<decltype(thunk)> func;
    };

    // Hook triggers when reading a book from the overworld.
    struct ReadBookReference {
        static bool thunk(RE::TESObjectBOOK* a1, RE::PlayerCharacter* a2);

        static void Install();

        static inline REL::Relocation<decltype(thunk)> func;
    };

    namespace stl {
        template <class T>
        void write_thunk_call(std::uintptr_t a_src) {
            auto& trampoline = SKSE::GetTrampoline();
            T::func = trampoline.write_call<5>(a_src, T::thunk);
        }

        template <typename TDest, typename TSource>
        constexpr auto write_vfunc() noexcept {
            REL::Relocation<std::uintptr_t> vtbl{TDest::VTABLE[0]};
            TSource::func = vtbl.write_vfunc(TSource::idx, TSource::Thunk);
        }

        template <typename T>
        constexpr auto write_vfunc(const REL::ID variant_id) noexcept {
            REL::Relocation<std::uintptr_t> vtbl{variant_id};
            T::func = vtbl.write_vfunc(T::idx, T::Thunk);
        }
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    static int hermitDaysCounter = 0;
    extern bool inApocrypha;

    void UpdateAvidReader(bool fromEvent);
    void LuckyHand();
    RE::ScrollItem* ProcessScrollsList(RE::TESLevItem* scrollsLeveledList);
    void ApplyMonkBonus();
    void UpdateHermit();
    void ResetHermitCounter();
    void EvaluateHermit();
    void UpdateCultist(bool fromEvent);
    void UpdateErudite(bool fromEvent);
}

#endif  // PHILOSOPHY_H