#pragma once



// C headers
#include <assert.h>
#include <math.h>
#include <stdint.h>



// C++ headers
#include <algorithm>
#include <array>
#include <chrono>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>



// Windows headers
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <ShellScalingApi.h>
#include <strsafe.h>



// DirectX headers
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <dxgi1_2.h>
#include <wrl/client.h>



// Direct2D/DirectWrite headers
#include <d2d1.h>
#include <d2d1_1.h>
#include <d2d1helper.h>
#include <dwrite.h>



// Library dependencies
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

#pragma comment(lib, "Shcore.lib")



#include "Ehm.h"
