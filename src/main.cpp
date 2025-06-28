#include <spdlog/sinks/basic_file_sink.h>

#include "Utility.h"
#include "Hooks.h"
#include "Horseman.h"
#include "Philosophy.h"
#include "Exploration.h"


void SKSEMessageHandler(SKSE::MessagingInterface::Message* message) {
    auto explorationEventProcessor = Exploration::EventProcessor::GetSingleton();
    auto horsemanEventProcessor = Horseman::EventProcessor::GetSingleton();
    auto philosophyEventProcessor = Philosophy::EventProcessor::GetSingleton();
    switch (message->type) {
        case SKSE::MessagingInterface::kDataLoaded: {
            RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(horsemanEventProcessor);
            RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(philosophyEventProcessor);
            RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(explorationEventProcessor);
            RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESFastTravelEndEvent>(
                explorationEventProcessor);
            RE::LocationDiscovery::GetEventSource()->AddEventSink<RE::LocationDiscovery::Event>(
                explorationEventProcessor);
            RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESActivateEvent>(explorationEventProcessor);
            RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESContainerChangedEvent>(
                explorationEventProcessor);
            RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESHitEvent>(explorationEventProcessor);
            RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESCombatEvent>(explorationEventProcessor);
            RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESCombatEvent>(philosophyEventProcessor);
            RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESMagicEffectApplyEvent>(
                philosophyEventProcessor);
        } break;
        case SKSE::MessagingInterface::kInputLoaded:
            SKSE::GetModCallbackEventSource()->AddEventSink(explorationEventProcessor);
            break;
        case SKSE::MessagingInterface::kPostLoadGame:
            //Horseman
            Horseman::UpdateJouster();

            //Exploration
            Exploration::UpdateCartographer(false);
            Exploration::InitCachedValues();

            //Philosophy
            Philosophy::UpdateAvidReader(false);
            Philosophy::UpdateErudite(false);
            Philosophy::UpdateCultist(false);
            break;
        case SKSE::MessagingInterface::kPostLoad:
        case SKSE::MessagingInterface::kNewGame:
        case SKSE::MessagingInterface::kSaveGame:
        default: 
            break;
    }
}

extern "C" [[maybe_unused]] __declspec(dllexport) bool SKSEPlugin_Load(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    SKSE::AllocTrampoline(249);

    SetupLog();
    spdlog::set_level(spdlog::level::info);
    SKSE::GetMessagingInterface()->RegisterListener(SKSEMessageHandler);
    SKSE::GetMessagingInterface()->RegisterListener("CustomSkills", [](SKSE::MessagingInterface::Message* msg) {
        CustomSkills::QueryCustomSkillsInterface(msg, PluginAPIStorage::get().customSkills);
        if (PluginAPIStorage::get().customSkills) {
            logger::info("CustomSkills API acquired successfully.");
        } else {
            logger::warn("Failed to acquire CustomSkills API.");
        }
    });
    
    Horseman::InstallHooks();
    Exploration::InstallHooks();
    Philosophy::InstallHooks();
    SharedHooks::InstallHooks();

    logger::info("Successfully loaded Firmament.dll!");

    return true;
}