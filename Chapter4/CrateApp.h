#pragma once

#ifndef HLSLTYPE_USE_MATRICES
#define HLSLTYPE_USE_MATRICES
#endif

#include "Common/framework.h"
#include "Common/d3dApp.h"
#include "Common/GeometryGenerator.h"
#include <ppl.h>
#include "Common/DDSTextureLoader.h"

#ifndef D3D12BOOK_CRATE_H
#define D3D12BOOK_CRATE_H

using namespace DirectX;
using namespace DirectX::PackedVector;

constexpr int MaxLights = 16;

struct Vertex
{
    float3 Pos;
    float3 Normal;
    float2 TexC;
};

cbuffer ObjectConstants _register(b0)
{
    float4x4 World = DirectXHelper::Math::Identity4X4();
    float4x4 WorldInvTranspose = DirectXHelper::Math::Identity4X4();
    float4x4 TexTransform = DirectXHelper::Math::Identity4X4();
};

cbuffer PassConstants _register(b2)
{
    float4x4 View = DirectXHelper::Math::Identity4X4();
    float4x4 InvView = DirectXHelper::Math::Identity4X4();
    float4x4 Proj = DirectXHelper::Math::Identity4X4();
    float4x4 InvProj = DirectXHelper::Math::Identity4X4();
    float4x4 ViewProj = DirectXHelper::Math::Identity4X4();
    float4x4 InvViewProj = DirectXHelper::Math::Identity4X4();
    float3 EyePosW = { 0.0f, 0.0f, 0.0f };
    float Gamma = 1.0f;
    float2 RenderTargetSize = { 0.0f, 0.0f };
    float2 InvRenderTargetSize = { 0.0f, 0.0f };
    float NearZ = 0.0f;
    float FarZ = 0.0f;
    float TotalTime = 0.0f;
    float DeltaTime = 0.0f;
    float UnscaledTotalTime = 0.0f;
    float UnscaledDeltaTime = 0.0f;
    float2 cbPerObjectPad2 = float2(0.0f, 0.0f);
    float4 AmbientLight = { 0.0f, 0.0f, 0.0f, 0.0f };
    DirectXHelper::Light Lights[MaxLights];
    int NumLights = 0;
};

struct FrameResource
{
public:
    ComPtr<ID3D12CommandAllocator> CmdListAlloc;

    std::unique_ptr<DirectXHelper::UploadBuffer<PassConstants>> PassCB = nullptr;
    std::unique_ptr<DirectXHelper::UploadBuffer<ObjectConstants>> ObjectCB = nullptr;
    std::unique_ptr<DirectXHelper::UploadBuffer<DirectXHelper::MaterialConstants>> MaterialCB = nullptr;

    UINT64 Fence = 0;

    FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount)
    {
        ThrowIfFailed(device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(CmdListAlloc.GetAddressOf())
        ));

        PassCB = std::make_unique<DirectXHelper::UploadBuffer<PassConstants>>(device, passCount, true);
        ObjectCB = std::make_unique<DirectXHelper::UploadBuffer<ObjectConstants>>(device, objectCount, true);
        MaterialCB = std::make_unique<DirectXHelper::UploadBuffer<DirectXHelper::MaterialConstants>>(device, materialCount, true);
    }

    FrameResource(const FrameResource&) = delete;
    FrameResource& operator=(const FrameResource&) = delete;
    ~FrameResource() {}
};

constexpr int gNumFrameResources = 3;

struct RenderItem
{
    float4x4 World = DirectXHelper::Math::Identity4X4();
    float4x4 TexTransform = DirectXHelper::Math::Identity4X4();

    int NumFramesDirty = gNumFrameResources;
    UINT ObjCBIndex = -1;

    DirectXHelper::Material* Mat = nullptr;
    DirectXHelper::MeshGeometry* Geo = nullptr;

    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    INT BaseVertexLocation = 0;

    RenderItem() = default;
};

enum class RenderLayer : int
{
    Opaque = 0,
    Count
};

class CrateApp : public D3DApp
{
private:
    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrFrameResource = nullptr;
    int mCurrFrameResourceIndex = 0;

    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

    ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap;
    ComPtr<ID3D12DescriptorHeap> mSamplerDescriptorHeap;

    std::unordered_map<std::wstring, std::unique_ptr<DirectXHelper::MeshGeometry>> mGeometries;
    std::unordered_map<std::wstring, std::unique_ptr<DirectXHelper::Material>> mMaterials;
    std::unordered_map<std::wstring, std::unique_ptr<DirectXHelper::Texture>> mTextures;
    std::unordered_map<std::wstring, ComPtr<ID3DBlob>> mShaders;
    std::unordered_map<std::wstring, ComPtr<ID3D12PipelineState>> mPSOs;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    std::vector<std::unique_ptr<RenderItem>> mAllRItems;
    std::vector<RenderItem*> mRItemLayer[(int)RenderLayer::Count];

    PassConstants mMainPassCB;

    bool mIsWireframe = false;

    float3 mEyePos = { 0.0f, 0.0f, 0.0f };
    float4x4 mView = DirectXHelper::Math::Identity4X4();
    float4x4 mProj = DirectXHelper::Math::Identity4X4();

    float mPhi = 1.5f * XM_PI;
    float mTheta = XM_PIDIV4;
    float mRadius = 3.0f;

    float mSunPhi = 1.25f * XM_PI;
    float mSunTheta = XM_PIDIV4;

    float mGamma = 1.0f;

    POINT mLastMousePos;

public:
    CrateApp(HINSTANCE hInstance);
    CrateApp(const CrateApp&) = delete;
    CrateApp& operator=(const CrateApp&) = delete;
    ~CrateApp();

    virtual bool Initialize(const D3DApp::D3DAPP_SETTINGS& settings) override;

    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

private:
    virtual void OnResize() override;
    virtual void Update(GameTimer<float>& gt) override;
    virtual void Draw(const GameTimer<float>& gt) override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

    void OnKeyboardInput(const GameTimer<float>& gt);
    void UpdateCamera(const GameTimer<float>& gt);
    void AnimateMaterials(const GameTimer<float>& gt);
    void UpdateObjectCBs(const GameTimer<float>& gt);
    void UpdateMaterialCBs(const GameTimer<float>& gt);
    void UpdateMainPassCB(const GameTimer<float>& gt);

    void LoadTextures();
    void BuildRootSignature();
    void BuildDescriptorHeaps();
    void BuildShadersAndInputLayout();
    void BuildShapeGeometry();
    void BuildPSOs();
    void BuildFrameResources();
    void BuildMaterials();
    void BuildRenderItems();
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& rItems);
};

CrateApp::CrateApp(HINSTANCE hInstance)
    : D3DApp(hInstance)
{
}

CrateApp::~CrateApp()
{
    if(md3dDevice != nullptr)
        FlushCommandQueue();
}

bool CrateApp::Initialize(const D3DApp::D3DAPP_SETTINGS& settings)
{
    if(!D3DApp::Initialize(settings))
        return false;

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    LoadTextures();
    BuildRootSignature();
    BuildDescriptorHeaps();
    BuildShadersAndInputLayout();
    BuildShapeGeometry();
    BuildMaterials();
    BuildRenderItems();
    BuildFrameResources();
    BuildPSOs();

    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdList[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(1, cmdList);

    FlushCommandQueue();

    return true;
}

LRESULT CrateApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

void CrateApp::OnResize()
{
    D3DApp::OnResize();

    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * XM_PI, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);
}

void CrateApp::Update(GameTimer<float>& gt)
{
    OnKeyboardInput(gt);
    UpdateCamera(gt);

    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

    if(mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
    {
        HANDLE eventHandle = CreateEventExW(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

    AnimateMaterials(gt);
    UpdateObjectCBs(gt);
    UpdateMaterialCBs(gt);
    UpdateMainPassCB(gt);
}

void CrateApp::Draw(const GameTimer<float>& gt)
{
    ID3D12CommandAllocator* cmdListAlloc = mCurrFrameResource->CmdListAlloc.Get();

    ThrowIfFailed(cmdListAlloc->Reset());
    ThrowIfFailed(mCommandList->Reset(cmdListAlloc, mPSOs[mIsWireframe ? L"opaque_wireframe" : L"opaque"].Get()));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    D3D12_RESOURCE_BARRIER rtTarget = CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    mCommandList->ResourceBarrier(1, &rtTarget);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = CurrentBackBufferView();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = DepthStencilView();
    mCommandList->ClearRenderTargetView(rtv, Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    mCommandList->OMSetRenderTargets(1, &rtv, true, &dsv);

    ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get(), mSamplerDescriptorHeap.Get() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    mCommandList->SetGraphicsRootDescriptorTable(1, mSamplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    mCommandList->SetGraphicsRootConstantBufferView(4, mCurrFrameResource->PassCB->Resource()->GetGPUVirtualAddress());

    DrawRenderItems(mCommandList.Get(), mRItemLayer[(int)RenderLayer::Opaque]);

    D3D12_RESOURCE_BARRIER rtPresent = CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );
    mCommandList->ResourceBarrier(1, &rtPresent);

    ThrowIfFailed(mCommandList->Close());

    ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    ThrowIfFailed(mSwapChain->Present(0, 0));
    mCurrentBackBuffer = (mCurrentBackBuffer + 1) % mSwapChainBufferCount;

    mCurrFrameResource->Fence = ++mCurrentFence;

    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void CrateApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;
    SetCapture(mhMainWnd);
}

void CrateApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void CrateApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    if((btnState & MK_LBUTTON) != 0)
    {
        float dx = XMConvertToRadians(0.25f * (x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f * (y - mLastMousePos.y));

        mPhi -= dx;
        mTheta -= dy;

        mTheta = DirectXHelper::Math::Clamp<float>(mTheta, 0.1f, XM_PI - 0.1f);
    }
    else if((btnState & MK_RBUTTON) != 0)
    {
        float dx = 0.05f * (x - mLastMousePos.x);
        float dy = 0.05f * (y - mLastMousePos.y);

        mRadius += dx - dy;

        mRadius = DirectXHelper::Math::Clamp<float>(mRadius, 1.5f, 10.0f);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

void CrateApp::OnKeyboardInput(const GameTimer<float>& gt)
{
    if(GetAsyncKeyState('1') & 0x8000)
        mIsWireframe = true;
    else
        mIsWireframe = false;

    const float dt = gt.UnscaledDeltaTime();

    if(GetAsyncKeyState(VK_LEFT) & 0x8000)
        mSunPhi -= 1.0f * dt;
    if(GetAsyncKeyState(VK_RIGHT) & 0x8000)
        mSunPhi += 1.0f * dt;
    if(GetAsyncKeyState(VK_UP) & 0x8000)
        mSunTheta -= 1.0f * dt;
    if(GetAsyncKeyState(VK_DOWN) & 0x8000)
        mSunTheta += 1.0f * dt;

    if(GetAsyncKeyState('2') & 0x8000)
        mGamma -= 0.5f * dt;
    if(GetAsyncKeyState('3') & 0x8000)
        mGamma += 0.5f * dt;

    mSunTheta = DirectXHelper::Math::Clamp(mSunTheta, 0.1f, XM_PIDIV2);
    mGamma = DirectXHelper::Math::Clamp(mGamma, 1.0f, 2.2f);
}

void CrateApp::UpdateCamera(const GameTimer<float>& gt)
{
    mEyePos.x = mRadius * sinf(mTheta) * cosf(mPhi);
    mEyePos.y = mRadius * cosf(mTheta);
    mEyePos.z = mRadius * sinf(mTheta) * sinf(mPhi);

    XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, view);
}

inline void CrateApp::AnimateMaterials(const GameTimer<float>& gt)
{
}

void CrateApp::UpdateObjectCBs(const GameTimer<float>& gt)
{
    DirectXHelper::UploadBuffer<ObjectConstants>* currObjectCB = mCurrFrameResource->ObjectCB.get();

    for(auto& e : mAllRItems)
    {
        if(e->NumFramesDirty > 0)
        {
            XMMATRIX world = XMLoadFloat4x4(&e->World);
            XMMATRIX worldInvTranspose = DirectXHelper::Math::InverseTranspose(world);
            XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

            ObjectConstants objConst = {};
            XMStoreFloat4x4(&objConst.World, XMMatrixTranspose(world));
            XMStoreFloat4x4(&objConst.WorldInvTranspose, XMMatrixTranspose(worldInvTranspose));
            XMStoreFloat4x4(&objConst.TexTransform, XMMatrixTranspose(texTransform));

            currObjectCB->CopyData(e->ObjCBIndex, objConst);

            e->NumFramesDirty--;
        }
    }
}

void CrateApp::UpdateMaterialCBs(const GameTimer<float>& gt)
{
    DirectXHelper::UploadBuffer<DirectXHelper::MaterialConstants>* currMaterialCB = mCurrFrameResource->MaterialCB.get();

    for(auto& e : mMaterials)
    {
        DirectXHelper::Material* mat = e.second.get();
        if(mat->NumFramesDirty > 0)
        {
            XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

            DirectXHelper::MaterialConstants matConstants = {};
            matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
            matConstants.FresnelR0 = mat->FresnelR0;
            matConstants.Roughness = mat->Roughness;
            XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

            currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

            mat->NumFramesDirty--;
        }
    }
}

void CrateApp::UpdateMainPassCB(const GameTimer<float>& gt)
{
    static float accTime = 0.0f;

    accTime += gt.DeltaTime();

    XMMATRIX view = XMLoadFloat4x4(&mView);
    XMMATRIX proj = XMLoadFloat4x4(&mProj);

    XMMATRIX viewProj = view * proj;
    XMVECTOR viewDet = XMMatrixDeterminant(view);
    XMMATRIX invView = XMMatrixInverse(&viewDet, view);
    XMVECTOR projDet = XMMatrixDeterminant(proj);
    XMMATRIX invProj = XMMatrixInverse(&projDet, proj);
    XMVECTOR viewProjDet = XMMatrixDeterminant(viewProj);
    XMMATRIX invViewProj = XMMatrixInverse(&viewProjDet, viewProj);

    XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
    mMainPassCB.EyePosW = mEyePos;
    mMainPassCB.Gamma = mGamma;
    mMainPassCB.RenderTargetSize = float2((float)mClientWidth, (float)mClientHeight);
    mMainPassCB.InvRenderTargetSize = float2(1.0f / mClientWidth, 1.0f / mClientHeight);
    mMainPassCB.NearZ = 1.0f;
    mMainPassCB.FarZ = 1000.0f;
    mMainPassCB.TotalTime = gt.TotalTime();
    mMainPassCB.UnscaledTotalTime = gt.UnscaledTotalTime();
    mMainPassCB.DeltaTime = gt.DeltaTime();
    mMainPassCB.UnscaledDeltaTime = gt.UnscaledDeltaTime();

    mMainPassCB.AmbientLight = { 0.1f, 0.1f, 0.1f, 1.0f };

    mMainPassCB.Lights[0].Type = (int)DirectXHelper::LIGHT_TYPE_DIRECTIONAL;
    XMVECTOR lightDir = -DirectXHelper::Math::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);
    XMStoreFloat3(&mMainPassCB.Lights[0].Direction, lightDir);
    mMainPassCB.Lights[0].Strength = { 0.9f, 0.9f, 0.9f };

    mMainPassCB.Lights[1].Type = (int)DirectXHelper::LIGHT_TYPE_POINT;
    mMainPassCB.Lights[1].Strength = { 0.1f, 0.9f, 0.1f };
    mMainPassCB.Lights[1].FalloffStart = 0.2f;
    mMainPassCB.Lights[1].FalloffEnd = 3.0f;
    mMainPassCB.Lights[1].Position = { 2.5f * sinf(accTime), 2.5f * cosf(accTime), 0.0f };

    mMainPassCB.Lights[2].Type = (int)DirectXHelper::LIGHT_TYPE_SPOTLIGHT;
    mMainPassCB.Lights[2].Strength = { 0.8f, 0.0f, 0.2f };
    mMainPassCB.Lights[2].FalloffStart = 0.1f;
    mMainPassCB.Lights[2].FalloffEnd = 5.0f;
    lightDir = XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f);
    lightDir = XMVector3Normalize(lightDir);
    XMStoreFloat3(&mMainPassCB.Lights[2].Direction, lightDir);
    mMainPassCB.Lights[2].Position = { -1.5f, 0.0f, 0.0f };
    mMainPassCB.Lights[2].SpotPower = 10.0f;

    mMainPassCB.NumLights = 3;

    DirectXHelper::UploadBuffer<PassConstants>* currPassCB = mCurrFrameResource->PassCB.get();
    currPassCB->CopyData(0, mMainPassCB);
}

void CrateApp::LoadTextures()
{
    std::unique_ptr<DirectXHelper::Texture> woodCrateTex = std::make_unique<DirectXHelper::Texture>();
    woodCrateTex->Name = L"woodCrateTex";
    woodCrateTex->Filename = L"Textures/WoodCrate01.dds";
    ThrowIfFailed(CreateDDSTextureFromFile12(
        md3dDevice.Get(),
        mCommandList.Get(),
        woodCrateTex->Filename.c_str(),
        woodCrateTex->Resource,
        woodCrateTex->UploadHeap
    ));

    mTextures[woodCrateTex->Name] = std::move(woodCrateTex);
}

void CrateApp::BuildRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE texTable;
    texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    CD3DX12_DESCRIPTOR_RANGE samplerTable;
    samplerTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

    CD3DX12_ROOT_PARAMETER slotRootParameter[5];

    slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
    slotRootParameter[1].InitAsDescriptorTable(1, &samplerTable, D3D12_SHADER_VISIBILITY_PIXEL);
    slotRootParameter[2].InitAsConstantBufferView(0);
    slotRootParameter[3].InitAsConstantBufferView(1);
    slotRootParameter[4].InitAsConstantBufferView(2);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(5, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

void CrateApp::BuildDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 1;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(mSrvDescriptorHeap.GetAddressOf())));

    CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    auto woodCrateTex = mTextures[L"woodCrateTex"]->Resource;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = woodCrateTex->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = woodCrateTex->GetDesc().MipLevels;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    md3dDevice->CreateShaderResourceView(woodCrateTex.Get(), &srvDesc, hDescriptor);

    D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
    samplerHeapDesc.NumDescriptors = 1;
    samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(mSamplerDescriptorHeap.GetAddressOf())));

    D3D12_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 16;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;

    md3dDevice->CreateSampler(&samplerDesc, mSamplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void CrateApp::BuildShadersAndInputLayout()
{
    mShaders[L"standardVS"] = DirectXHelper::CompileShader(
        L"Shaders\\TexLighting.hlsl",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "VS",
        "vs_5_0",
        0, 0
    );

    mShaders[L"opaquePS"] = DirectXHelper::CompileShader(
        L"Shaders\\TexLighting.hlsl",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "PS",
        "ps_5_0",
        0, 0
    );

    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
}

void CrateApp::BuildShapeGeometry()
{
    GeometryGenerator geoGen;
    GeometryGenerator::MeshData<std::uint16_t> box = geoGen.CreateBox<std::uint16_t>(1.0f, 1.0f, 1.0f, 3);

    DirectXHelper::MeshGeometry::SubmeshGeometry boxSubmesh = {};
    boxSubmesh.IndexCount = (UINT)box.Indices.size();
    boxSubmesh.StartIndexLocation = 0;
    boxSubmesh.BaseVertexLocation = 0;

    std::vector<Vertex> vertices(box.Vertices.size());

    for(std::size_t i = 0; i < box.Vertices.size(); i++)
    {
        vertices[i].Pos = box.Vertices[i].Position;
        vertices[i].Normal = box.Vertices[i].Normal;
        vertices[i].TexC = box.Vertices[i].TexC;
    }

    std::vector<std::uint16_t> indices = box.GetIndices16();

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    auto geo = std::make_unique<DirectXHelper::MeshGeometry>();

    geo->Name = L"boxGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, geo->VertexBufferCPU.GetAddressOf()));
    ThrowIfFailed(D3DCreateBlob(ibByteSize, geo->IndexBufferCPU.GetAddressOf()));
    memcpy_s(geo->VertexBufferCPU->GetBufferPointer(), vbByteSize, vertices.data(), vbByteSize);
    memcpy_s(geo->IndexBufferCPU->GetBufferPointer(), ibByteSize, indices.data(), ibByteSize);

    ThrowIfFailed(DirectXHelper::CreateDefaultBuffer(
        md3dDevice.Get(),
        mCommandList.Get(),
        vertices.data(),
        vbByteSize,
        geo->VertexBufferGPU.GetAddressOf(),
        geo->VertexUploader.GetAddressOf()
    ));
    ThrowIfFailed(DirectXHelper::CreateDefaultBuffer(
        md3dDevice.Get(),
        mCommandList.Get(),
        indices.data(),
        ibByteSize,
        geo->IndexBufferGPU.GetAddressOf(),
        geo->IndexUploader.GetAddressOf()
    ));

    geo->VertexByteStride = sizeof(Vertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    geo->DrawArgs[L"box"] = boxSubmesh;

    mGeometries[geo->Name] = std::move(geo);
}

void CrateApp::BuildPSOs()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc = {};
    opaquePsoDesc.InputLayout.pInputElementDescs = mInputLayout.data();
    opaquePsoDesc.InputLayout.NumElements = mInputLayout.size();
    opaquePsoDesc.pRootSignature = mRootSignature.Get();
    opaquePsoDesc.VS.pShaderBytecode = (BYTE*)mShaders[L"standardVS"]->GetBufferPointer();
    opaquePsoDesc.VS.BytecodeLength = mShaders[L"standardVS"]->GetBufferSize();
    opaquePsoDesc.PS.pShaderBytecode = (BYTE*)mShaders[L"opaquePS"]->GetBufferPointer();
    opaquePsoDesc.PS.BytecodeLength = mShaders[L"opaquePS"]->GetBufferSize();
    opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    opaquePsoDesc.SampleMask = UINT_MAX;
    opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    opaquePsoDesc.NumRenderTargets = 1;
    opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
    opaquePsoDesc.SampleDesc.Count = mMsaaState ? mMsaaSampleCount : 1;
    opaquePsoDesc.SampleDesc.Quality = mMsaaState ? (mMsaaQuality - 1) : 0;
    opaquePsoDesc.DSVFormat = mDepthStencilFormat;

    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(mPSOs[L"opaque"].GetAddressOf())));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = opaquePsoDesc;
    opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;

    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(mPSOs[L"opaque_wireframe"].GetAddressOf())));
}

void CrateApp::BuildFrameResources()
{
    for(int i = 0; i < gNumFrameResources; i++)
    {
        mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(), 1, (UINT)mAllRItems.size(), (UINT)mMaterials.size()));
    }
}

void CrateApp::BuildMaterials()
{
    std::unique_ptr<DirectXHelper::Material> woodCrate = std::make_unique<DirectXHelper::Material>();
    woodCrate->Name = L"woodCrate";
    woodCrate->MatCBIndex = 0;
    woodCrate->DiffuseSrvHeapIndex = 0;
    woodCrate->NumFramesDirty = gNumFrameResources;
    woodCrate->DiffuseAlbedo = float4(1.0f, 1.0f, 1.0f, 1.0f);
    woodCrate->FresnelR0 = float3(0.05f, 0.05f, 0.05f);
    woodCrate->Roughness = 0.2f;

    mMaterials[L"woodCrate"] = std::move(woodCrate);
}

void CrateApp::BuildRenderItems()
{
    std::unique_ptr<RenderItem> boxRitem = std::make_unique<RenderItem>();
    boxRitem->World = DirectXHelper::Math::Identity4X4();
    boxRitem->ObjCBIndex = 0;
    boxRitem->Mat = mMaterials[L"woodCrate"].get();
    boxRitem->Geo = mGeometries[L"boxGeo"].get();
    boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    boxRitem->IndexCount = boxRitem->Geo->DrawArgs[L"box"].IndexCount;
    boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs[L"box"].StartIndexLocation;
    boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs[L"box"].BaseVertexLocation;
    mAllRItems.push_back(std::move(boxRitem));

    for(auto& e : mAllRItems)
    {
        mRItemLayer[(int)RenderLayer::Opaque].push_back(e.get());
    }
}

void CrateApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& rItems)
{
    UINT objCBSize = DirectXHelper::CalcConstantBufferByteSize<ObjectConstants>();
    UINT matCBSize = DirectXHelper::CalcConstantBufferByteSize<DirectXHelper::MaterialConstants>();
    ID3D12Resource* objectCB = mCurrFrameResource->ObjectCB->Resource();
    ID3D12Resource* materialCB = mCurrFrameResource->MaterialCB->Resource();

    for(size_t i = 0; i < rItems.size(); i++)
    {
        RenderItem* ri = rItems[i];

        D3D12_VERTEX_BUFFER_VIEW vbv = ri->Geo->VertexBufferView();
        D3D12_INDEX_BUFFER_VIEW ibv = ri->Geo->IndexBufferView();
        cmdList->IASetVertexBuffers(0, 1, &vbv);
        cmdList->IASetIndexBuffer(&ibv);
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

        CD3DX12_GPU_DESCRIPTOR_HANDLE tex(
            mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()
        );
        tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvDescriptorSize);

        cmdList->SetGraphicsRootDescriptorTable(0, tex);
        cmdList->SetGraphicsRootConstantBufferView(2, objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBSize);
        cmdList->SetGraphicsRootConstantBufferView(3, materialCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex * matCBSize);

        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }
}

#endif