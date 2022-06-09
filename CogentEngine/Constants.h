#pragma once
#include <cstdint>

// Dimensions
constexpr uint32_t SCREEN_WIDTH			= 1910;
constexpr uint32_t SCREEN_HEIGHT		= 1061;


/// Engine Constants
constexpr uint32_t MAX_CBUFFER_SIZE			= 1024 * 32;
constexpr uint32_t FRAME_BUFFER_COUNT		= 3;
constexpr uint32_t RENDER_TARGET_COUNT		= 32;
constexpr uint32_t HEAPSIZE					= 4096;

constexpr float BG_COLOR[] = { 0.0f, 0.2f, 0.3f, 1.0f };

enum class AreaLightType
{
    Sphere,
    Disc,
    Rect
};



// Typedefs
typedef uint32_t uint32;