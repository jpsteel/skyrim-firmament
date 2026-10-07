#ifndef HOOKS_H
#define HOOKS_H

#include "Exploration.h"
#include "Utility.h"

namespace SharedHooks {
    void InstallHooks();
    void InstallSkillUseGuard();

    static void Update(RE::PlayerCharacter* player, float delta);
    static inline REL::Relocation<decltype(Update)> _Update;
}

#endif  // HOOKS_H