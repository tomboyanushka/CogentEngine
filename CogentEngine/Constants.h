#pragma once
#include <cstdint>
#include <string>

// Dimensions
constexpr uint32_t SCREEN_WIDTH			= 1920;
constexpr uint32_t SCREEN_HEIGHT		= 1080;


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

static const std::string defaultDiffuseFile = "../../Assets/Textures/default/diffuse.png";
static const std::string defaultNormalFile = "../../Assets/Textures/default/normal.png";
static const std::string defaultMetalFile = "../../Assets/Textures/default/metal.png";
static const std::string defaultRoughnessFile = "../../Assets/Textures/default/roughness.png";


// Typedefs
typedef uint32_t uint32;