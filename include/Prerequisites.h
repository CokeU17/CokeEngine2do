#pragma once

// Librerías estándar usadas en varias partes del motor
#include <string>
#include <sstream>
#include <vector>
#include <windows.h>
#include <xnamath.h>
#include <thread>

// Librerías principales de DirectX 11
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include "Resource.h"
#include "resource.h"

// Third Party Libraries

//--------------------------------------------------------------------------------------
// MACROS
//--------------------------------------------------------------------------------------

// Libera recursos COM de forma segura y evita errores en if/else
#define SAFE_RELEASE(x)        \
do {                           \
    if ((x) != nullptr) {      \
        (x)->Release();        \
        (x) = nullptr;         \
    }                          \
} while (0)

// Mensaje simple para depuración al crear recursos
#define MESSAGE(classObj, method, state)                                      \
{                                                                             \
    std::wostringstream os_;                                                  \
    os_ << classObj << L"::" << method                                        \
        << L" : " << L"[CREATION OF RESOURCE: " << state << L"]\n";           \
    OutputDebugStringW(os_.str().c_str());                                    \
}

// Mensaje de error para ver problemas en la ventana Output
#define ERROR(classObj, method, errorMSG)                                     \
{                                                                             \
    try {                                                                     \
        std::wostringstream os_;                                              \
        os_ << L"ERROR : " << classObj << L"::" << method                     \
            << L" : " << errorMSG << L"\n";                                   \
        OutputDebugStringW(os_.str().c_str());                                \
    } catch (...) {                                                           \
        OutputDebugStringW(L"Failed to log error message.\n");                \
    }                                                                         \
}

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------

// Vértice básico con posición y coordenadas de textura
struct SimpleVertex
{
    XMFLOAT3 Pos;
    XMFLOAT2 Tex;
};

// Constant buffer para datos que casi no cambian
struct CBNeverChanges
{
    XMMATRIX mView;
};

// Constant buffer para actualización por cambio de ventana
struct CBChangeOnResize
{
    XMMATRIX mProjection;
};

// Constant buffer para datos que cambian cada frame
struct CBChangesEveryFrame
{
    XMMATRIX mWorld;
    XMFLOAT4 vMeshColor;
};

// Tipos de extensión soportados para texturas
enum ExtensionType
{
    DDS = 0,
    PNG = 1,
    JPG = 2
};

// Tipos de shader usados por el motor
enum ShaderType
{
    VERTEX_SHADER = 0,
    PIXEL_SHADER = 1
};