#pragma once

#include "REL/Relocation.h"

namespace RE::Offset {

    namespace Actor {
        constexpr auto ActorValueModifiedCallbacks = REL::ID(403905);
        constexpr auto ComputeMovementType = REL::ID(37943);
        constexpr auto ForceUpdateCachedMovementType = REL::ID(37941);
    }

    namespace BGSSaveLoadGame {
        inline constexpr REL::ID Singleton{static_cast<std::uint64_t>(403330)};
    }

    namespace BSScript {
        namespace ObjectBindPolicy {
            inline constexpr REL::ID GetInitialPropertyValues(static_cast<std::uint64_t>(104176));
        }    }
}
