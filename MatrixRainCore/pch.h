#pragma once

// C++ Standard Library
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

// Windows SDK
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

// Direct3D 11
#include <d3d11_1.h>
#include <dxgi1_2.h>

// Direct2D and DirectWrite
#include <d2d1_1.h>
#include <d2d1helper.h>
#include <dwrite.h>

// Link required libraries
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
