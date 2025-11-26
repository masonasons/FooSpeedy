// Precompiled header for foo_dsp_speedy
#pragma once

#define _WIN32_WINNT _WIN32_WINNT_WIN7
#define WINVER _WIN32_WINNT_WIN7

// Windows headers - winsock2 must come before windows.h
#include <SDKDDKVer.h>
#include <winsock2.h>   // Must be before windows.h
#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>
#pragma comment(lib, "winmm.lib")
#include <mmsystem.h>   // For timeGetTime
#include <objbase.h>    // For COM/OLE
#include <shlobj.h>     // For shell interfaces

// Standard library
#include <vector>
#include <memory>
#include <cmath>
#include <algorithm>

// foobar2000 SDK
#include "lib/foobar2000_SDK/foobar2000/SDK/foobar2000.h"

// Resource IDs
#include "src/resource.h"
