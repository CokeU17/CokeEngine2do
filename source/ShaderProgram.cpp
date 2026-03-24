#include "ShaderProgram.h"
#include "Device.h"
#include "DeviceContext.h"

HRESULT
ShaderProgram::init(Device& device,
    const std::string& fileName,
    std::vector<D3D11_INPUT_ELEMENT_DESC> Layout) {
    if (!device.m_device) {
        ERROR("ShaderProgram", "init", "Device is null.");
        return E_POINTER;
    }

    if (fileName.empty()) {
        ERROR("ShaderProgram", "init", "Shader file name is empty.");
        return E_INVALIDARG;
    }

    if (Layout.empty()) {
        ERROR("ShaderProgram", "init", "Input layout is empty.");
        return E_INVALIDARG;
    }

    m_shaderFileName = fileName;

    // Primero creamos vertex shader porque de ahí sale la data para el input layout
    HRESULT hr = CreateShader(device, ShaderType::VERTEX_SHADER);
    if (FAILED(hr)) {
        ERROR("ShaderProgram", "init", "Error creating vertex shader.");
        return hr;
    }

    // Después se crea el layout de entrada con la data compilada del VS
    hr = CreateInputLayout(device, Layout);
    if (FAILED(hr)) {
        ERROR("ShaderProgram", "init", "Error creating input layout.");
        return hr;
    }

    // Finalmente se crea el pixel shader
    hr = CreateShader(device, ShaderType::PIXEL_SHADER);
    if (FAILED(hr)) {
        ERROR("ShaderProgram", "init", "Error creating pixel shader.");
        return hr;
    }

    return S_OK;
}

HRESULT
ShaderProgram::CreateInputLayout(Device& device,
    std::vector<D3D11_INPUT_ELEMENT_DESC> Layout) {
    if (!m_vertexShaderData) {
        ERROR("ShaderProgram", "CreateInputLayout", "Vertex shader data is null.");
        return E_POINTER;
    }

    if (!device.m_device) {
        ERROR("ShaderProgram", "CreateInputLayout", "Device is null.");
        return E_POINTER;
    }

    if (Layout.empty()) {
        ERROR("ShaderProgram", "CreateInputLayout", "Input layout is empty.");
        return E_INVALIDARG;
    }

    HRESULT hr = m_inputLayout.init(device, Layout, m_vertexShaderData);

    // Ya no ocupamos guardar la data compilada del VS en este punto
    SAFE_RELEASE(m_vertexShaderData);

    if (FAILED(hr)) {
        ERROR("ShaderProgram", "CreateInputLayout", "Error creating input layout.");
        return hr;
    }

    return S_OK;
}

HRESULT
ShaderProgram::CreateShader(Device& device, ShaderType type) {
    if (!device.m_device) {
        ERROR("ShaderProgram", "CreateShader", "Device is null.");
        return E_POINTER;
    }

    if (m_shaderFileName.empty()) {
        ERROR("ShaderProgram", "CreateShader", "Shader file name is empty.");
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    ID3DBlob* shaderData = nullptr;

    const char* shaderEntryPoint = (type == ShaderType::PIXEL_SHADER) ? "PS" : "VS";
    const char* shaderModel = (type == ShaderType::PIXEL_SHADER) ? "ps_4_0" : "vs_4_0";

    // Compilamos el shader desde archivo
    hr = CompileShaderFromFile(const_cast<char*>(m_shaderFileName.c_str()),
        shaderEntryPoint,
        shaderModel,
        &shaderData);

    if (FAILED(hr)) {
        ERROR("ShaderProgram", "CreateShader", "Error compiling shader file.");
        return hr;
    }

    // Creamos el objeto shader correspondiente
    if (type == ShaderType::PIXEL_SHADER) {
        hr = device.CreatePixelShader(shaderData->GetBufferPointer(),
            shaderData->GetBufferSize(),
            nullptr,
            &m_PixelShader);
    }
    else {
        hr = device.CreateVertexShader(shaderData->GetBufferPointer(),
            shaderData->GetBufferSize(),
            nullptr,
            &m_VertexShader);
    }

    if (FAILED(hr)) {
        ERROR("ShaderProgram", "CreateShader", "Error creating shader object.");
        SAFE_RELEASE(shaderData);
        return hr;
    }

    // Guardamos la data compilada según el tipo de shader
    if (type == ShaderType::PIXEL_SHADER) {
        SAFE_RELEASE(m_pixelShaderData);
        m_pixelShaderData = shaderData;
    }
    else {
        SAFE_RELEASE(m_vertexShaderData);
        m_vertexShaderData = shaderData;
    }

    return S_OK;
}

HRESULT
ShaderProgram::CreateShader(Device& device,
    ShaderType type,
    const std::string& fileName) {
    if (!device.m_device) {
        ERROR("ShaderProgram", "CreateShader", "Device is null.");
        return E_POINTER;
    }

    if (fileName.empty()) {
        ERROR("ShaderProgram", "CreateShader", "Shader file name is empty.");
        return E_INVALIDARG;
    }

    m_shaderFileName = fileName;

    HRESULT hr = CreateShader(device, type);
    if (FAILED(hr)) {
        ERROR("ShaderProgram", "CreateShader", "Error creating shader from file.");
        return hr;
    }

    return S_OK;
}

HRESULT
ShaderProgram::CompileShaderFromFile(char* szFileName,
    LPCSTR szEntryPoint,
    LPCSTR szShaderModel,
    ID3DBlob** ppBlobOut) {
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
    // En debug dejamos información extra para revisar shaders más fácil
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob = nullptr;

    hr = D3DX11CompileFromFile(szFileName,
        nullptr,
        nullptr,
        szEntryPoint,
        szShaderModel,
        dwShaderFlags,
        0,
        nullptr,
        ppBlobOut,
        &pErrorBlob,
        nullptr);

    if (FAILED(hr)) {
        if (pErrorBlob) {
            OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
            ERROR("ShaderProgram", "CompileShaderFromFile", "Shader compilation failed.");
            SAFE_RELEASE(pErrorBlob); // Liberamos memoria del error
        }
        else {
            ERROR("ShaderProgram", "CompileShaderFromFile", "Shader compilation failed with no error message.");
        }

        return hr;
    }

    SAFE_RELEASE(pErrorBlob);
    return S_OK;
}

void
ShaderProgram::render(DeviceContext& deviceContext) {
    // Activamos shaders y layout dentro del pipeline
    if (!m_VertexShader || !m_PixelShader || !m_inputLayout.m_inputLayout) {
        ERROR("ShaderProgram", "render", "Shaders or InputLayout not initialized.");
        return;
    }

    m_inputLayout.render(deviceContext);
    deviceContext.m_deviceContext->VSSetShader(m_VertexShader, nullptr, 0);
    deviceContext.m_deviceContext->PSSetShader(m_PixelShader, nullptr, 0);
}

void
ShaderProgram::render(DeviceContext& deviceContext, ShaderType type) {
    if (!deviceContext.m_deviceContext) {
        ERROR("ShaderProgram", "render", "DeviceContext is null.");
        return;
    }

    switch (type) {
    case VERTEX_SHADER:
        deviceContext.m_deviceContext->VSSetShader(m_VertexShader, nullptr, 0);
        break;

    case PIXEL_SHADER:
        deviceContext.m_deviceContext->PSSetShader(m_PixelShader, nullptr, 0);
        break;

    default:
        ERROR("ShaderProgram", "render", "Unsupported shader type.");
        break;
    }
}

void
ShaderProgram::destroy() {
    // Liberación de recursos usados por el shader program
    SAFE_RELEASE(m_VertexShader);
    m_inputLayout.destroy();
    SAFE_RELEASE(m_PixelShader);
    SAFE_RELEASE(m_vertexShaderData);
    SAFE_RELEASE(m_pixelShaderData);
}