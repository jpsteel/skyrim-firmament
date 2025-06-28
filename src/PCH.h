#pragma once

// This file is required.

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include <SKSE/API.h>
#include <SKSE/Logger.h>
#include <SKSE/Interfaces.h>
#include <SKSE/Events.h>
#include <REL/Relocation.h>
#include <SKSE/Trampoline.h>
#include <SKSE/RegistrationSet.h>
#include "RE/B/BSFixedString.h"
#include "RE/I/IMenu.h"
#include "RE/U/UI.h"
#include "RE/G/GFxMovieView.h"
#include "RE/G/GFxMovieDef.h"
#include "RE/G/GFxValue.h"
#include "RE/B/BSScaleformManager.h"
#include "RE/I/InterfaceStrings.h"

using namespace std::literals;

constexpr uint32_t DATA_VERSION = 1;

namespace logger = SKSE::log;
using namespace std::literals;
namespace util {
    [[nodiscard]] constexpr int ascii_tolower(int ch) noexcept {
        if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
        return ch;
    }

    struct iless {
        using is_transparent = int;

        template <std::ranges::contiguous_range S1, std::ranges::contiguous_range S2>
            requires(std::is_same_v<std::ranges::range_value_t<S1>, char> &&
                     std::is_same_v<std::ranges::range_value_t<S2>, char>)
        constexpr bool operator()(S1&& a_str1, S2&& a_str2) const {
            std::size_t count = std::ranges::size(a_str2);
            const std::size_t len1 = std::ranges::size(a_str1);
            const bool shorter = len1 < count;
            if (shorter) count = len1;

            if (count) {
                const char* p1 = std::ranges::data(a_str1);
                const char* p2 = std::ranges::data(a_str2);

                do {
                    const int ch1 = ascii_tolower(*p1++);
                    const int ch2 = ascii_tolower(*p2++);
                    if (ch1 != ch2) return ch1 < ch2;
                } while (--count);
            }

            return shorter;
        }
    };

    using SKSE::stl::report_and_fail;

    template <class T>
    using istring_map = std::map<std::string, T, iless>;
}