#pragma once
#include <cmath>

namespace coastal
{
enum class ActionDirection { Front, Right, Back, Left };

// Source direction in character-local coordinates: +X forward, +Y right.
// Vertical/coincident/invalid sources have no horizontal bearing: use front.
inline ActionDirection CharacterActionDirection(double Forward, double Right)
{
    if (!std::isfinite(Forward) || !std::isfinite(Right)) return ActionDirection::Front;
    if (std::abs(Forward) >= std::abs(Right))
        return Forward < 0.0 ? ActionDirection::Back : ActionDirection::Front;
    return Right < 0.0 ? ActionDirection::Left : ActionDirection::Right;
}
}
