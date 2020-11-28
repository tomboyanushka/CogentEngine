#pragma once
#include <cstdint>

// Dimensions
constexpr uint32_t SCREEN_WIDTH			= 1920.0f;
constexpr uint32_t SCREEN_HEIGHT		= 1080.0f;


/// Engine Constants
constexpr uint32_t MAX_CBUFFER_SIZE			= 1024 * 32;
constexpr uint32_t FRAME_BUFFER_COUNT		= 3;
constexpr uint32_t RENDER_TARGET_COUNT		= 32;
constexpr uint32_t HEAPSIZE					= 4096;


// Typedefs
typedef uint32_t uint32;