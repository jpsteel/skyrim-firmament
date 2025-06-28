#pragma once

namespace Data {
    namespace Horseman {
        [[nodiscard]] inline float GetLevel() {
            auto level = RE::TESForm::LookupByEditorID<RE::TESGlobal>("SkillHorsemanLevel");
            return level ? level->value : 0.0f;
        }
    }

    namespace Exploration {
        [[nodiscard]] inline float GetLevel() {
            auto level = RE::TESForm::LookupByEditorID<RE::TESGlobal>("SkillExplorationLevel");
            return level ? level->value : 0.0f;
        }
    }

    namespace Philosophy {
        [[nodiscard]] inline float GetLevel() {
            auto level = RE::TESForm::LookupByEditorID<RE::TESGlobal>("SkillPhilosophyLevel");
            return level ? level->value : 0.0f;
        }
    }
}