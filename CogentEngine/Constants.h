#pragma once
#include <cstdint>

constexpr uint32_t c_MaxConstBufferSize = 1024 * 32; //16 kb
typedef uint32_t uint32;

// Engine Constants

// Triple buffered
constexpr uint32_t cFrameBufferCount = 3;
constexpr uint32_t cMaxRenderTargetCount = 16;

constexpr uint32_t c_HeapSize = 4096;