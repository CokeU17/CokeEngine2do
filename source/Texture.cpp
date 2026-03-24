#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Texture.h"
#include "Device.h"
#include "DeviceContext.h"

// Inicialización desde archivo (PNG, JPG, DDS)
HRESULT
Texture::init(Device& device,
    const std::string& textureName,
    ExtensionType extensionType) {

    // Validaciones básicas
    if (!device.m_device) {
        ERROR("Texture", "init", "Device no inicializado");
        return E_POINTER;
    }

    if (textureName.empty()) {
        ERROR("Texture", "init", "Nombre de textura vacío");
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    switch (extensionType) {

        // ================= DDS =================
    case DDS: {
        m_textureName = textureName + ".dds";

        // DirectX carga directo DDS
        hr = D3DX11CreateShaderResourceViewFromFile(
            device.m_device,
            m_textureName.c_str(),
            nullptr,
            nullptr,
            &m_textureFromImg,
            nullptr
        );

        if (FAILED(hr)) {
            ERROR("Texture", "init", "Error cargando DDS");
            return hr;
        }
        break;
    }

            // ================= PNG / JPG =================
    case PNG:
    case JPG: {

        m_textureName = textureName + (extensionType == PNG ? ".png" : ".jpg");

        int width, height, channels;

        // Carga de imagen con stb
        unsigned char* data = stbi_load(
            m_textureName.c_str(),
            &width, &height, &channels, 4
        );

        if (!data) {
            ERROR("Texture", "init", "Error cargando imagen");
            return E_FAIL;
        }

        // Descripción de textura
        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width = width;
        textureDesc.Height = height;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        // Datos iniciales
        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = data;
        initData.SysMemPitch = width * 4;

        // Crear textura en GPU
        hr = device.CreateTexture2D(&textureDesc, &initData, &m_texture);

        stbi_image_free(data); // liberar RAM

        if (FAILED(hr)) {
            ERROR("Texture", "init", "Error creando textura GPU");
            return hr;
        }

        // Crear vista para shader
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        hr = device.m_device->CreateShaderResourceView(
            m_texture,
            &srvDesc,
            &m_textureFromImg
        );

        SAFE_RELEASE(m_texture); // ya no se necesita directa

        if (FAILED(hr)) {
            ERROR("Texture", "init", "Error creando SRV");
            return hr;
        }

        break;
    }

    default:
        ERROR("Texture", "init", "Formato no soportado");
        return E_INVALIDARG;
    }

    return hr;
}