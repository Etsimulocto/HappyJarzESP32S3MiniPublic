#pragma once

#include <Arduino.h>

// Shared LED types/constants are in a header so Arduino's sketch preprocessor
// sees them before generating prototypes for functions that use Rgb.
static constexpr uint8_t LED_COUNT = 4;
struct Rgb { uint8_t r, g, b; };
