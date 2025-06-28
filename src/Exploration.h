#ifndef EXPLORATION_H
#define EXPLORATION_H

namespace Exploration {

    class EventProcessor : 
    public RE::BSTEventSink<RE::LocationDiscovery::Event>,
                           public RE::BSTEventSink<RE::TESActivateEvent>,
                           public RE::BSTEventSink<SKSE::ModCallbackEvent>,
                           public RE::BSTEventSink<RE::TESFastTravelEndEvent>,
                           public RE::BSTEventSink<RE::MenuOpenCloseEvent>,
                           public RE::BSTEventSink<RE::TESContainerChangedEvent>,
                           public RE::BSTEventSink<RE::TESHitEvent>,
                           public RE::BSTEventSink<RE::TESCombatEvent> {
    public:
        static EventProcessor* GetSingleton() {
            static EventProcessor instance;
            return &instance;
        }

        RE::BSEventNotifyControl ProcessEvent(const RE::LocationDiscovery::Event* event,
                                              RE::BSTEventSource<RE::LocationDiscovery::Event>* eventSource) override;
        RE::BSEventNotifyControl ProcessEvent(const RE::TESActivateEvent* event,
                                              RE::BSTEventSource<RE::TESActivateEvent>* eventSource) override;
        RE::BSEventNotifyControl ProcessEvent(const SKSE::ModCallbackEvent* event,
                                              RE::BSTEventSource<SKSE::ModCallbackEvent>* eventSource) override;
        RE::BSEventNotifyControl ProcessEvent(const RE::TESFastTravelEndEvent* event,
                                              RE::BSTEventSource<RE::TESFastTravelEndEvent>* eventSource) override;
        RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* event,
                                              RE::BSTEventSource<RE::MenuOpenCloseEvent>* eventSource) override;
        RE::BSEventNotifyControl ProcessEvent(const RE::TESContainerChangedEvent* event,
                                              RE::BSTEventSource<RE::TESContainerChangedEvent>* eventSource) override;
        RE::BSEventNotifyControl ProcessEvent(const RE::TESHitEvent* event,
                                              RE::BSTEventSource<RE::TESHitEvent>* eventSource) override;
        RE::BSEventNotifyControl ProcessEvent(const RE::TESCombatEvent* event,
                                              RE::BSTEventSource<RE::TESCombatEvent>* eventSource) override;
    };

    void InstallHooks();

    static inline void ProcessEvent(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_sink,
                                    RE::BSAnimationGraphEvent* a_event,
                                    RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource);
    static RE::BSEventNotifyControl ProcessEvent_PC(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_sink,
                                                    RE::BSAnimationGraphEvent* a_event,
                                                    RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource);
    static inline REL::Relocation<decltype(ProcessEvent_PC)> _ProcessEvent_PC;

    static bool fastTravelEnabled;
    static bool playerIsMining;
    static bool stopAddingOre;
    static float originalSurvivalEnabled;
    extern bool initializedCachedValues;
    extern bool appliedDungeonDelver;

    void UpdateCartographer(bool fromEvent);
    void PilgrimClearSkies();
    void DiscoverRandomLocation();
    bool IsPlayerInDungeon();
    void UpdateCamperWellRested();
    void UpdateDungeonDelver();
    void InitCachedValues();
}

#endif  // EXPLORATION_H