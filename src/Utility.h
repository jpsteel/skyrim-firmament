#ifndef UTILITY_H
#define UTILITY_H

#include "CustomSkills/Interfaces.h"

#include <nlohmann/json.hpp>
#include <unordered_set>

namespace logger = SKSE::log;

class PluginAPIStorage {
public:
    static PluginAPIStorage& get() {
        static PluginAPIStorage singleton{};
        return singleton;
    }

    CustomSkills::CustomSkillsInterface* customSkills;
};

CustomSkills::CustomSkillsInterface* GetCustomSkillsInterface();

void SetupLog();

RE::ActorValue LookupActorValueByName(const char* av_name);
std::int32_t QueryStat(const char* name);
int RandomInt(int min, int max);
RE::BSTArray<RE::ObjectRefHandle> GetUndiscoveredMapMarkers();
bool HasEffect(RE::Actor* actor, RE::EffectSetting* effect);
float GetWarmthRating(RE::Actor* actor);
bool IsMarkerInPlayerWorld(RE::TESWorldSpace* a_refWorld);
RE::TESWorldSpace* GetRootWorld(RE::TESWorldSpace* a_World);
RE::BGSLocation* GetRootLocation(RE::BGSLocation* a_loc);
std::string GetPlayerRootWorldName();
bool IsPlayersMount(const RE::Actor* actor);

#endif  // UTILITY_H