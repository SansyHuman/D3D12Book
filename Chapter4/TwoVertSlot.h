#pragma once

#include "Common/framework.h"
#include "Common/d3dApp.h"

#ifndef D3D12BOOK_TWOVERTSLOT_H
#define D3D12BOOK_TWOVERTSLOT_H

using namespace DirectX;
using namespace DirectX::PackedVector;

struct VPosData
{
    XMFLOAT3 Pos;  
};

struct VColorData
{
    XMFLOAT4 Color;
};

cbuffer ObjectConstants _register(b0)
{
    XMFLOAT4X4 WorldViewProj = DirectXHelper::Math::Identity4X4();
};

class TwoVertSlot : public D3DApp
{
private:
    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

    std::unique_ptr<DirectXHelper::UploadBuffer<ObjectConstants>> mObjectCB = nullptr;
    std::unique_ptr<DirectXHelper::MeshGeometry> mBoxGeo = nullptr;
    std::unique_ptr<DirectXHelper::MeshGeometry> mBoxCol = nullptr;

    ComPtr<ID3DBlob> mvsByteCode;
    ComPtr<ID3DBlob> mpsByteCode;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    ComPtr<ID3D12PipelineState> mPSO = nullptr;

    XMFLOAT4X4 mWorld = DirectXHelper::Math::Identity4X4();
    XMFLOAT4X4 mView = DirectXHelper::Math::Identity4X4();
    XMFLOAT4X4 mProj = DirectXHelper::Math::Identity4X4();

    float mTheta = 1.5f * XM_PI;
    float mPhi = XM_PIDIV4;
    float mRadius = 5.0f;

    POINT mLastMousePos;

public:
    TwoVertSlot(HINSTANCE hInstance);
    TwoVertSlot(const TwoVertSlot&) = delete;
    TwoVertSlot& operator=(const TwoVertSlot&) = delete;
    ~TwoVertSlot();

    virtual bool Initialize(const D3DApp::D3DAPP_SETTINGS& settings) override;

    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

private:
    virtual void OnResize() override;
    virtual void Update(GameTimer<float>& gt) override;
    virtual void Draw(const GameTimer<float>& gt) override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

    void BuildDescriptorHeaps();
    void BuildConstantBuffers();
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildBoxGeometry();
    void BuildPSO();
};

TwoVertSlot::TwoVertSlot(HINSTANCE hInstance)
    : D3DApp(hInstance)
{
}

TwoVertSlot::~TwoVertSlot()
{
}

bool TwoVertSlot::Initialize(const D3DApp::D3DAPP_SETTINGS& settings)
{
    if(!D3DApp::Initialize(settings))
        return false;

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    BuildDescriptorHeaps();
    BuildConstantBuffers();
    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildBoxGeometry();
    BuildPSO();

    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdList[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(1, cmdList);

    FlushCommandQueue();

    return true;
}

LRESULT TwoVertSlot::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
    case WM_MOUSEWHEEL:
        double zDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
        mTimer.SetTimescale(mTimer.GetTimescale() + zDelta * 0.05);
        return 0;
    }

    return D3DApp::MsgProc(hwnd, msg, wParam, lParam);
}

void TwoVertSlot::OnResize()
{
    D3DApp::OnResize();

    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * XM_PI, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);
}

void TwoVertSlot::Update(GameTimer<float>& gt)
{
    float x = mRadius * sinf(mPhi) * cosf(mTheta);
    float y = mRadius * cosf(mPhi);
    float z = mRadius * sinf(mPhi) * sinf(mTheta);

    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, view);

    XMMATRIX world = XMLoadFloat4x4(&mWorld);
    XMMATRIX proj = XMLoadFloat4x4(&mProj);
    XMMATRIX worldViewProj = world * view * proj;

    world = world * XMMatrixRotationY(XM_PIDIV2 * gt.DeltaTime());
    XMStoreFloat4x4(&mWorld, world);

    ObjectConstants objCons = {};
    XMStoreFloat4x4(&objCons.WorldViewProj, XMMatrixTranspose(worldViewProj));
    mObjectCB->CopyData(0, objCons);
}

void TwoVertSlot::Draw(const GameTimer<float>& gt)
{
    ThrowIfFailed(mDirectCmdListAlloc->Reset());
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    D3D12_RESOURCE_BARRIER rtTarget = CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    mCommandList->ResourceBarrier(1, &rtTarget);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CurrentBackBufferView();
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = DepthStencilView();

    mCommandList->ClearRenderTargetView(rtvHandle, Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);


    mCommandList->OMSetRenderTargets(1, &rtvHandle, true, &dsvHandle);

    ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
    mCommandList->SetDescriptorHeaps(1, descriptorHeaps);

    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    D3D12_VERTEX_BUFFER_VIEW vbv = mBoxGeo->VertexBufferView();
    D3D12_VERTEX_BUFFER_VIEW vcbv = mBoxCol->VertexBufferView();
    D3D12_INDEX_BUFFER_VIEW ibv = mBoxGeo->IndexBufferView();

    mCommandList->IASetVertexBuffers(0, 1, &vbv);
    mCommandList->IASetVertexBuffers(1, 1, &vcbv);
    mCommandList->IASetIndexBuffer(&ibv);
    mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());

    DirectXHelper::MeshGeometry::SubmeshGeometry box = mBoxGeo->DrawArgs[L"상자"];
    mCommandList->DrawIndexedInstanced(
        box.IndexCount,
        1, box.StartIndexLocation, box.BaseVertexLocation, 0
    );

    D3D12_RESOURCE_BARRIER rtPresent = CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );
    mCommandList->ResourceBarrier(1, &rtPresent);

    ThrowIfFailed(mCommandList->Close());

    ID3D12CommandList* cmdList[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdList), cmdList);

    ThrowIfFailed(mSwapChain->Present(0, 0));
    mCurrentBackBuffer = (mCurrentBackBuffer + 1) % mSwapChainBufferCount;

    FlushCommandQueue();
}

void TwoVertSlot::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;
    SetCapture(mhMainWnd);
}

void TwoVertSlot::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void TwoVertSlot::OnMouseMove(WPARAM btnState, int x, int y)
{
    if((btnState & MK_LBUTTON) != 0)
    {
        float dx = XMConvertToRadians(0.25f * (x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f * (y - mLastMousePos.y));

        mTheta -= dx;
        mPhi -= dy;

        mPhi = DirectXHelper::Math::Clamp<float>(mPhi, 0.1f, XM_PI - 0.1f);
    }
    else if((btnState & MK_RBUTTON) != 0)
    {
        float dx = 0.005f * (x - mLastMousePos.x);
        float dy = 0.005f * (y - mLastMousePos.y);

        mRadius += dx - dy;

        mRadius = DirectXHelper::Math::Clamp<float>(mRadius, 3.0f, 15.0f);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

void TwoVertSlot::BuildDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.NumDescriptors = 2;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;

    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(mCbvHeap.GetAddressOf())));
}

void TwoVertSlot::BuildConstantBuffers()
{
    mObjectCB = std::make_unique<DirectXHelper::UploadBuffer<ObjectConstants>>(md3dDevice.Get(), 1, true);

    UINT objCBSize = DirectXHelper::CalcConstantBufferByteSize<ObjectConstants>();
    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();

    int boxCBufIndex = 0;

    cbAddress += boxCBufIndex * objCBSize;

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = cbAddress;
    cbvDesc.SizeInBytes = objCBSize;

    md3dDevice->CreateConstantBufferView(&cbvDesc, mCbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void TwoVertSlot::BuildRootSignature()
{
    CD3DX12_ROOT_PARAMETER slotRootParameter[1];

    CD3DX12_DESCRIPTOR_RANGE cbvTable;
    cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> error = nullptr;

    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), error.GetAddressOf());

    if(error != nullptr)
    {
        OutputDebugStringA((char*)error->GetBufferPointer());
    }

    ThrowIfFailed(hr);
    ThrowIfFailed(md3dDevice->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void TwoVertSlot::BuildShadersAndInputLayout()
{
    mvsByteCode = DirectXHelper::CompileShader(
        L"Shaders\\Box.hlsl",
        nullptr,
        nullptr,
        "VS",
        "vs_5_0",
        0, 0
    );

    mpsByteCode = DirectXHelper::CompileShader(
        L"Shaders\\Box.hlsl",
        nullptr,
        nullptr,
        "PS",
        "ps_5_0",
        0, 0
    );


    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
}

void TwoVertSlot::BuildBoxGeometry()
{
    std::array<VPosData, 8> vertices =
    {
        VPosData(XMFLOAT3(-1.0f, -1.0f, -1.0f)),
        VPosData(XMFLOAT3(-1.0f, +1.0f, -1.0f)),
        VPosData(XMFLOAT3(+1.0f, +1.0f, -1.0f)),
        VPosData(XMFLOAT3(+1.0f, -1.0f, -1.0f)),
        VPosData(XMFLOAT3(-1.0f, -1.0f, +1.0f)),
        VPosData(XMFLOAT3(-1.0f, +1.0f, +1.0f)),
        VPosData(XMFLOAT3(+1.0f, +1.0f, +1.0f)),
        VPosData(XMFLOAT3(+1.0f, -1.0f, +1.0f))
    };

    std::array<VColorData, 8> colors =
    {
        VColorData(XMFLOAT4(Colors::White)),
        VColorData(XMFLOAT4(Colors::Black)),
        VColorData(XMFLOAT4(Colors::Red)),
        VColorData(XMFLOAT4(Colors::Green)),
        VColorData(XMFLOAT4(Colors::Blue)),
        VColorData(XMFLOAT4(Colors::Yellow)),
        VColorData(XMFLOAT4(Colors::Cyan)),
        VColorData(XMFLOAT4(Colors::Magenta))
    };

    std::array<std::uint16_t, 36> indices =
    {
        // front face
        0, 1, 2,
        0, 2, 3,

        // back face
        4, 6, 5,
        4, 7, 6,

        // left face
        4, 5, 1,
        4, 1, 0,

        // right face
        3, 2, 6,
        3, 6, 7,

        // top face
        1, 5, 6,
        1, 6, 2,

        // bottom face
        4, 0, 3,
        4, 3, 7
    };

    constexpr UINT vbByteSize = (UINT)vertices.size() * sizeof(VPosData);
    constexpr UINT vcbByteSize = (UINT)colors.size() * sizeof(VColorData);
    constexpr UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    mBoxGeo = std::make_unique<DirectXHelper::MeshGeometry>();
    mBoxGeo->Name = L"상자";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, mBoxGeo->VertexBufferCPU.GetAddressOf()));
    ThrowIfFailed(D3DCreateBlob(ibByteSize, mBoxGeo->IndexBufferCPU.GetAddressOf()));
    memcpy_s(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vbByteSize, vertices.data(), vbByteSize);
    memcpy_s(mBoxGeo->IndexBufferCPU->GetBufferPointer(), ibByteSize, indices.data(), ibByteSize);

    ThrowIfFailed(DirectXHelper::CreateDefaultBuffer(
        md3dDevice.Get(),
        mCommandList.Get(),
        vertices.data(),
        vbByteSize,
        mBoxGeo->VertexBufferGPU.GetAddressOf(),
        mBoxGeo->VertexUploader.GetAddressOf()
    ));
    ThrowIfFailed(DirectXHelper::CreateDefaultBuffer(
        md3dDevice.Get(),
        mCommandList.Get(),
        indices.data(),
        ibByteSize,
        mBoxGeo->IndexBufferGPU.GetAddressOf(),
        mBoxGeo->IndexUploader.GetAddressOf()
    ));

    mBoxGeo->VertexByteStride = sizeof(VPosData);
    mBoxGeo->VertexBufferByteSize = vbByteSize;
    mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
    mBoxGeo->IndexBufferByteSize = ibByteSize;

    DirectXHelper::MeshGeometry::SubmeshGeometry submesh = {};
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    mBoxGeo->DrawArgs[L"상자"] = submesh;

    mBoxCol = std::make_unique<DirectXHelper::MeshGeometry>();
    mBoxCol->Name = L"상자";

    ThrowIfFailed(D3DCreateBlob(vcbByteSize, mBoxCol->VertexBufferCPU.GetAddressOf()));
    memcpy_s(mBoxCol->VertexBufferCPU->GetBufferPointer(), vcbByteSize, colors.data(), vcbByteSize);

    ThrowIfFailed(DirectXHelper::CreateDefaultBuffer(
        md3dDevice.Get(),
        mCommandList.Get(),
        colors.data(),
        vcbByteSize,
        mBoxCol->VertexBufferGPU.GetAddressOf(),
        mBoxCol->VertexUploader.GetAddressOf()
    ));

    mBoxCol->VertexByteStride = sizeof(VColorData);
    mBoxCol->VertexBufferByteSize = vcbByteSize;
}

void TwoVertSlot::BuildPSO()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout.pInputElementDescs = mInputLayout.data();
    psoDesc.InputLayout.NumElements = mInputLayout.size();
    psoDesc.pRootSignature = mRootSignature.Get();
    psoDesc.VS.pShaderBytecode = (BYTE*)mvsByteCode->GetBufferPointer();
    psoDesc.VS.BytecodeLength = mvsByteCode->GetBufferSize();
    psoDesc.PS.pShaderBytecode = (BYTE*)mpsByteCode->GetBufferPointer();
    psoDesc.PS.BytecodeLength = mpsByteCode->GetBufferSize();
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = mBackBufferFormat;
    psoDesc.SampleDesc.Count = mMsaaState ? mMsaaSampleCount : 1;
    psoDesc.SampleDesc.Quality = mMsaaState ? (mMsaaQuality - 1) : 0;
    psoDesc.DSVFormat = mDepthStencilFormat;

    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(mPSO.GetAddressOf())));
}

#endif