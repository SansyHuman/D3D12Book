#pragma once

#include "framework.h"
#include "concepts.h"

#ifndef D3D12BOOK_D3DUTIL_H
#define D3D12BOOK_D3DUTIL_H

#ifndef cbuffer
#define cbuffer struct
#endif

#ifndef _register
#define _register(slot)
#endif

using namespace DirectX;

template<class T> requires Derived<T, IUnknown>
inline void ReleaseCom(T*& obj)
{
    if(obj)
    {
        obj->Release();
        obj = nullptr;
    }
}

inline std::wstring AnsiToWString(const std::string& str)
{
    WCHAR buffer[512] = { 0 };
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

#ifndef ThrowIfFailed
#define ThrowIfFailed(x) \
{                        \
    HRESULT hr__ = (x);    \
    if(FAILED(hr__))       \
    {                    \
        std::wstring fnm = AnsiToWString(__FILE__); \
        throw DirectXHelper::DxException(hr__, L#x, fnm, __LINE__);\
    }\
}
#endif

namespace DirectXHelper
{
    namespace Math
    {
        template <class T> requires Floating<T>
        inline T Clamp(T value, T min, T max)
        {
            return fmax(min, fmin(value, max));
        }

        template <class T> requires Integer<T>
        inline T Clamp(T value, T min, T max)
        {
            return value < min ? min : (value > max ? max : value);
        }

        inline constexpr XMFLOAT4X4 Identity4X4()
        {
            return XMFLOAT4X4(
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1
            );
        }

        inline float RandF(float min, float max)
        {
            return min + ((float)rand() / RAND_MAX) * (max - min);
        }

        inline int Rand(int min, int max)
        {
            return min + rand() % ((max - min) + 1);
        }

        inline XMVECTOR ComputeNormal(FXMVECTOR p0, FXMVECTOR p1, FXMVECTOR p2)
        {
            XMVECTOR u = p1 - p0;
            XMVECTOR v = p2 - p0;

            return XMVector3Normalize(XMVector3Cross(u, v));
        }

        inline XMMATRIX InverseTranspose(CXMMATRIX M)
        {
            XMMATRIX A = M;
            A.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

            XMVECTOR det = XMMatrixDeterminant(A);
            return XMMatrixTranspose(XMMatrixInverse(&det, A));
        }

        inline XMVECTOR SphericalToCartesian(float r, float theta, float phi)
        {
            return DirectX::XMVectorSet(
                r * sinf(theta) * cosf(phi),
                r * cosf(theta),
                r * sinf(theta) * sinf(phi),
                1.0f);
        }
    }

    class DxException
    {
    public:
        DxException() = default;

        DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber)
        {
            ErrorCode = hr;
            FunctionName = functionName;
            Filename = filename;
            LineNumber = lineNumber;
        }

        std::wstring ToString() const
        {
            _com_error err(ErrorCode);
            std::wstring msg = err.ErrorMessage();

            return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " +
                msg;
        }

        HRESULT ErrorCode = S_OK;
        std::wstring FunctionName;
        std::wstring Filename;
        int LineNumber = -1;
    };

    template<class T>
    inline HRESULT CheckFeatureSupport(
            ID3D12Device* device,
            D3D12_FEATURE feature,
            T& featureSupportData
    )
    {
        return device->CheckFeatureSupport(
                feature,
                &featureSupportData,
                sizeof(T)
        );
    }

    template <class Allocator, class List>
    requires Derived<Allocator, ID3D12CommandAllocator> && Derived<List, ID3D12CommandList>
    inline HRESULT CreateCommandAllocatorAndList(
        ID3D12Device* device,
        UINT nodeMask,
        D3D12_COMMAND_LIST_TYPE type,
        ID3D12PipelineState* pInitialState,
        Allocator** ppAllocator,
        List** ppList
    )
    {
        HRESULT hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(ppAllocator));
        if(FAILED(hr))
            return hr;

        hr = device->CreateCommandList(nodeMask, type, *ppAllocator, pInitialState, IID_PPV_ARGS(ppList));

        return hr;
    }

    template <class Default, class Upload>
    requires Derived<Default, ID3D12Resource> && Derived<Upload, ID3D12Resource>
    inline HRESULT CreateDefaultBuffer(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const void* initData,
        UINT64 byteSize,
        Default** ppDefaultBuffer,
        Upload** ppUploadBuffer
    )
    {
        D3D12_HEAP_PROPERTIES defaultBufferProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        D3D12_RESOURCE_DESC defaultBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);

        HRESULT hr = device->CreateCommittedResource(
            &defaultBufferProperties,
            D3D12_HEAP_FLAG_NONE,
            &defaultBufferDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(ppDefaultBuffer)
        );
        if(FAILED(hr))
            return hr;

        D3D12_HEAP_PROPERTIES uploadBufferProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

        hr = device->CreateCommittedResource(
            &uploadBufferProperties,
            D3D12_HEAP_FLAG_NONE,
            &defaultBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(ppUploadBuffer)
        );
        if(FAILED(hr))
            return hr;

        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = initData;
        subresourceData.RowPitch = byteSize;
        subresourceData.SlicePitch = byteSize;

        D3D12_RESOURCE_BARRIER defaultBufferCopy = CD3DX12_RESOURCE_BARRIER::Transition(
            *ppDefaultBuffer,
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_COPY_DEST
        );
        cmdList->ResourceBarrier(1, &defaultBufferCopy);

        UpdateSubresources<1>(cmdList, *ppDefaultBuffer, *ppUploadBuffer, 0, 0, 1, &subresourceData);

        D3D12_RESOURCE_BARRIER defaultBufferRead = CD3DX12_RESOURCE_BARRIER::Transition(
            *ppDefaultBuffer,
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_GENERIC_READ
        );
        cmdList->ResourceBarrier(1, &defaultBufferRead);

        return hr;
    }

    inline UINT CalcConstantBufferByteSize(UINT byteSize)
    {
        return (byteSize + 255) & ~255;
    }

    template <class Data>
    inline UINT CalcConstantBufferByteSize()
    {
        return (sizeof(Data) + 255) & ~255;
    }

    template <class Data>
    class UploadBuffer
    {
    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
        BYTE* mMappedData = nullptr;

        UINT mElementByteSize = 0;
        UINT mBufferWidth = 0;
        bool mIsConstantBuffer = false;

    public:
        UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer)
            : mIsConstantBuffer(isConstantBuffer)
        {
            mElementByteSize = sizeof(Data);
            if(isConstantBuffer)
                mElementByteSize = CalcConstantBufferByteSize(mElementByteSize);

            mBufferWidth = mElementByteSize * elementCount;
            D3D12_HEAP_PROPERTIES uploadBufferProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            D3D12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(mBufferWidth);
            ThrowIfFailed(device->CreateCommittedResource(
                &uploadBufferProperties,
                D3D12_HEAP_FLAG_NONE,
                &uploadBufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(mUploadBuffer.GetAddressOf())
            ));

            ThrowIfFailed(mUploadBuffer->Map(0, nullptr, (void**)&mMappedData));
        }

        UploadBuffer(const UploadBuffer&) = delete;
        UploadBuffer& operator=(const UploadBuffer&) = delete;
        ~UploadBuffer()
        {
            if(mUploadBuffer != nullptr)
                mUploadBuffer->Unmap(0, nullptr);

            mMappedData = nullptr;
        }

        ID3D12Resource* Resource() const
        {
            return mUploadBuffer.Get();
        }

        void CopyData(UINT elementIndex, const Data& data)
        {
            UINT memoryIndex = elementIndex * mElementByteSize;
            memcpy_s(&mMappedData[memoryIndex], mBufferWidth - memoryIndex, &data, sizeof(Data));
        }
    };

    inline Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
        const std::wstring& fileName,
        const D3D_SHADER_MACRO* pDefines,
        ID3DInclude* pInclude,
        const std::string& entrypoint,
        const std::string& target,
        UINT compileFlags,
        UINT advancedFlags
    )
    {
#if defined(DEBUG) | defined(_DEBUG)
        compileFlags = compileFlags | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        Microsoft::WRL::ComPtr<ID3DBlob> code;
        Microsoft::WRL::ComPtr<ID3DBlob> errors;
        HRESULT hr = D3DCompileFromFile(
            fileName.c_str(),
            pDefines,
            pInclude,
            entrypoint.c_str(),
            target.c_str(),
            compileFlags,
            advancedFlags,
            code.GetAddressOf(),
            errors.GetAddressOf()
        );

        if(errors != nullptr)
        {
            OutputDebugStringA((char*)errors->GetBufferPointer());
        }

        ThrowIfFailed(hr);

        return code;
    }

    inline HRESULT LoadBinary(const std::wstring& fileName, ID3DBlob** ppBin)
    {
        std::ifstream fin(fileName, std::ios::binary);

        fin.seekg(0, std::ios_base::end);
        std::ifstream::pos_type size = (int)fin.tellg();
        fin.seekg(0, std::ios_base::beg);

        HRESULT hr = D3DCreateBlob(size, ppBin);
        if(FAILED(hr))
            return hr;

        fin.read((char*)((*ppBin)->GetBufferPointer()), size);
        fin.close();

        return hr;
    }

    struct MeshGeometry
    {
        struct SubmeshGeometry
        {
            UINT IndexCount = 0;
            UINT StartIndexLocation = 0;
            UINT BaseVertexLocation = 0;
            BoundingBox Bounds;
        };

        std::wstring Name;

        Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
        Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU = nullptr;

        Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

        Microsoft::WRL::ComPtr<ID3D12Resource> VertexUploader = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> IndexUploader = nullptr;

        UINT VertexByteStride = 0;
        UINT VertexBufferByteSize = 0;
        DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
        UINT IndexBufferByteSize = 0;

        std::unordered_map<std::wstring, SubmeshGeometry> DrawArgs;

        D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const
        {
            D3D12_VERTEX_BUFFER_VIEW vbv;
            vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
            vbv.StrideInBytes = VertexByteStride;
            vbv.SizeInBytes = VertexBufferByteSize;

            return vbv;
        }

        D3D12_INDEX_BUFFER_VIEW IndexBufferView() const
        {
            D3D12_INDEX_BUFFER_VIEW ibv;
            ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
            ibv.Format = IndexFormat;
            ibv.SizeInBytes = IndexBufferByteSize;

            return ibv;
        }

        void DisposeUploaders()
        {
            VertexUploader.Reset();
            IndexUploader.Reset();
        }
    };

    struct Material
    {
        std::wstring Name;

        int MatCBIndex = -1;
        int DiffuseSrvHeapIndex = -1;

        int NumFramesDirty = 0;

        XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
        XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
        float Roughness = 0.25f;
        XMFLOAT4X4 MatTransform = Math::Identity4X4();
    };

    struct MaterialConstants
    {
        XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
        XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
        float Roughness = 0.25f;
        XMFLOAT4X4 MatTransform = Math::Identity4X4();
    };

    enum LIGHT_TYPE
    {
        LIGHT_TYPE_DIRECTIONAL = 0,
        LIGHT_TYPE_POINT = 1,
        LIGHT_TYPE_SPOTLIGHT = 2
    };

    struct Light
    {
        int Type;
        XMFLOAT3 cbPerObjectPad1;
        XMFLOAT3 Strength;
        float FalloffStart;
        XMFLOAT3 Direction;
        float FalloffEnd;
        XMFLOAT3 Position;
        float SpotPower;
    };

    struct Texture
    {
        std::wstring Name;
        std::wstring Filename;

        Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;
    };
}

#endif //D3D12BOOK_D3DUTIL_H
