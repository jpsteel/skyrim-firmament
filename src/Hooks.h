#ifndef HOOKS_H
#define HOOKS_H

#include "Exploration.h"
#include "Utility.h"
#include <xbyak/xbyak.h>

namespace SharedHooks {
    void InstallHooks();
    static void Update(RE::PlayerCharacter* player, float delta);
    static inline REL::Relocation<decltype(Update)> _Update;
}

#endif  // HOOKS_H