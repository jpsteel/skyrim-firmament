#ifndef HORSEMAN_H
#define HORSEMAN_H

namespace Horseman {
    class EventProcessor : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
    public:
        static EventProcessor* GetSingleton() {
            static EventProcessor instance;
            return &instance;
        }

        RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* event,
                                              RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;
    };

    void InstallHooks();

    static void ProcessHit(RE::Actor* victim, RE::HitData& hitData);

    static inline REL::Relocation<decltype(ProcessHit)> _ProcessHit;

    // Modify move speed
    static void MoveSpeedPatch();

    // Update cached movement when speed mult is modified (for enchantments/potions/etc)
    static void SpeedMultModifiedPatch();

    static void SpeedMultModifiedCallback(RE::Actor* a_actor, RE::ActorValue a_actorValue, float a_originalValue,
                                          float a_modValue, RE::TESObjectREFR* a_cause);

    static float GetSpeedMult(const RE::Actor* a_actor);

    inline static REL::Relocation<decltype(&GetSpeedMult)> _GetScale;
    inline static REL::Relocation<decltype(&SpeedMultModifiedCallback)> _SpeedMultModifiedCallback;

    static inline void ProcessEvent(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_sink,
                                    RE::BSAnimationGraphEvent* a_event,
                                    RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource);

    void UpdateJouster();

    static RE::BSEventNotifyControl ProcessEvent_NPC(
        RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_sink,
                                        RE::BSAnimationGraphEvent* a_event,
                                        RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource);
    static RE::BSEventNotifyControl ProcessEvent_PC(
        RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_sink,
                                       RE::BSAnimationGraphEvent* a_event,
                                       RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource);

    static inline REL::Relocation<decltype(ProcessEvent_NPC)> _ProcessEvent_NPC;
    static inline REL::Relocation<decltype(ProcessEvent_PC)> _ProcessEvent_PC;
}


#endif  // HORSEMAN_H