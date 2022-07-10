#pragma once

#ifndef HLSLTYPE_USE_MATRICES
#define HLSLTYPE_USE_MATRICES
#endif

#include "Common/framework.h"
#include "Common/d3dApp.h"
#include "Common/GeometryGenerator.h"
#include "Common/DDSTextureLoader.h"
#include <ppl.h>
#include <sstream>

#ifndef D3D12BOOK_STENCILAPP_H
#define D3D12BOOK_STENCILAPP_H

using namespace DirectX;
using namespace DirectX::PackedVector;

constexpr int MaxLights = 16;

struct Vertex
{
    float3 Pos;
    float3 Normal;
    float2 TexC;

    Vertex() = default;

    Vertex(float x, float y, float z, float nx, float ny, float nz, float u, float t)
        : Pos(x, y, z), Normal(nx, ny, nz), TexC(u, t)
    {
    }
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
    float FogStart;
    float FogRange;
    float4 FogColor;
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
    AlphaTested,
    Mirrors,
    OpaqueReflected,
    AlphaTestedReflected,
    Transparent,
    Shadow,
    Count
};

class StencilApp : public D3DApp
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

    int mMaxSrvHeapSize = 0;
    int mMaxMaterialNumber = 0;
    int mMaxRenderItemNumber = 0;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    RenderItem* mWavesRItem = nullptr;

    std::vector<std::unique_ptr<RenderItem>> mAllRItems;
    std::vector<RenderItem*> mRItemLayer[(int)RenderLayer::Count];

    RenderItem* mSkullRitem = nullptr;
    RenderItem* mReflectedSkullRitem = nullptr;
    RenderItem* mShadowedSkullRitem = nullptr;

    PassConstants mMainPassCB;
    PassConstants mReflectedPassCB;

    float3 mSkullTranslation = { 0.0f, 1.0f, -5.0f };

    float3 mEyePos = { 0.0f, 0.0f, 0.0f };
    float4x4 mView = DirectXHelper::Math::Identity4X4();
    float4x4 mProj = DirectXHelper::Math::Identity4X4();

    float mPhi = 1.24f * XM_PI;
    float mTheta = 0.42f * XM_PI;
    float mRadius = 12.0f;

    float mSunPhi = 1.25f * XM_PI;
    float mSunTheta = XM_PIDIV4;

    float mGamma = 2.2f;

    POINT mLastMousePos;

public:
    StencilApp(HINSTANCE hInstance);
    StencilApp(const StencilApp&) = delete;
    StencilApp& operator=(const StencilApp&) = delete;
    ~StencilApp();

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
    void UpdateReflectedPassCB(const GameTimer<float>& gt);

    void LoadTextures();
    void BuildRootSignature();
    void BuildDescriptorHeaps();
    void BuildShadersAndInputLayout();
    void BuildGeometry();
    void BuildPSOs();
    void BuildFrameResources();
    void BuildMaterials();
    void BuildRenderItems();
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& rItems);

    void AddTexture(DDS_TEXTURE& texture);
    void AddMaterial(LPCWSTR name, LPCWSTR diffuseTexture, DirectXHelper::MaterialConstants& matConst);
    RenderItem* AddRenderItem(
        RenderLayer layer,
        LPCWSTR geometry,
        LPCWSTR subgeometry,
        LPCWSTR material,
        float4x4& worldTransform,
        float4x4& textureTransform,
        D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    );
};

StencilApp::StencilApp(HINSTANCE hInstance)
    : D3DApp(hInstance)
{
}

StencilApp::~StencilApp()
{
    if(md3dDevice != nullptr)
        FlushCommandQueue();
}

bool StencilApp::Initialize(const D3DApp::D3DAPP_SETTINGS& settings)
{
    if(!D3DApp::Initialize(settings))
        return false;

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    LoadTextures();
    BuildRootSignature();
    BuildDescriptorHeaps();
    BuildShadersAndInputLayout();
    BuildGeometry();
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

LRESULT StencilApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

void StencilApp::OnResize()
{
    D3DApp::OnResize();

    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * XM_PI, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);
}

void StencilApp::Update(GameTimer<float>& gt)
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
    UpdateReflectedPassCB(gt);
}

void StencilApp::Draw(const GameTimer<float>& gt)
{
    ID3D12CommandAllocator* cmdListAlloc = mCurrFrameResource->CmdListAlloc.Get();

    ThrowIfFailed(cmdListAlloc->Reset());
    ThrowIfFailed(mCommandList->Reset(cmdListAlloc, mPSOs[L"opaque"].Get()));

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

    UINT passCBByteSize = DirectXHelper::CalcConstantBufferByteSize<PassConstants>();

    // Opaque
    mCommandList->SetGraphicsRootConstantBufferView(4, mCurrFrameResource->PassCB->Resource()->GetGPUVirtualAddress());
    mCommandList->SetPipelineState(mPSOs[L"opaque"].Get());
    DrawRenderItems(mCommandList.Get(), mRItemLayer[(int)RenderLayer::Opaque]);

    // AlphaTested
    mCommandList->SetPipelineState(mPSOs[L"alphaTested"].Get());
    DrawRenderItems(mCommandList.Get(), mRItemLayer[(int)RenderLayer::AlphaTested]);

    // Mark visible mirror pixels in stencil buffer
    mCommandList->OMSetStencilRef(1);
    mCommandList->SetPipelineState(mPSOs[L"markStencilMirrors"].Get());
    DrawRenderItems(mCommandList.Get(), mRItemLayer[(int)RenderLayer::Mirrors]);

    // Reflections
    mCommandList->SetGraphicsRootConstantBufferView(4, mCurrFrameResource->PassCB->Resource()->GetGPUVirtualAddress() + passCBByteSize);
    mCommandList->SetPipelineState(mPSOs[L"drawOpaqueStencilReflections"].Get());
    DrawRenderItems(mCommandList.Get(), mRItemLayer[(int)RenderLayer::OpaqueReflected]);

    mCommandList->SetPipelineState(mPSOs[L"drawAlphaTestedStencilReflections"].Get());
    DrawRenderItems(mCommandList.Get(), mRItemLayer[(int)RenderLayer::AlphaTestedReflected]);

    // Restore main pass and stencil ref
    mCommandList->SetGraphicsRootConstantBufferView(4, mCurrFrameResource->PassCB->Resource()->GetGPUVirtualAddress());
    mCommandList->OMSetStencilRef(0);

    // Transparent
    mCommandList->SetPipelineState(mPSOs[L"transparent"].Get());
    DrawRenderItems(mCommandList.Get(), mRItemLayer[(int)RenderLayer::Transparent]);

    // Shadow
    mCommandList->SetPipelineState(mPSOs[L"shadow"].Get());
    DrawRenderItems(mCommandList.Get(), mRItemLayer[(int)RenderLayer::Shadow]);

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

void StencilApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;
    SetCapture(mhMainWnd);
}

void StencilApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void StencilApp::OnMouseMove(WPARAM btnState, int x, int y)
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

        mRadius = DirectXHelper::Math::Clamp<float>(mRadius, 5.0f, 150.0f);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

void StencilApp::OnKeyboardInput(const GameTimer<float>& gt)
{
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

    if(GetAsyncKeyState('A') & 0x8000)
        mSkullTranslation.x -= 1.0f * dt;
    if(GetAsyncKeyState('D') & 0x8000)
        mSkullTranslation.x += 1.0f * dt;
    if(GetAsyncKeyState('W') & 0x8000)
        mSkullTranslation.y += 1.0f * dt;
    if(GetAsyncKeyState('S') & 0x8000)
        mSkullTranslation.y -= 1.0f * dt;

    mSkullTranslation.y = DirectXHelper::Math::Clamp(mSkullTranslation.y, 0.0f, 10.0f);

    XMMATRIX skullRotate = XMMatrixRotationY(XM_PIDIV2);
    XMMATRIX skullScale = XMMatrixScaling(0.45f, 0.45f, 0.45f);
    XMMATRIX skullOffset = XMMatrixTranslation(mSkullTranslation.x, mSkullTranslation.y, mSkullTranslation.z);
    XMMATRIX skullWorld = skullScale * skullRotate * skullOffset;
    XMStoreFloat4x4(&mSkullRitem->World, skullWorld);

    XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    XMMATRIX R = XMMatrixReflect(mirrorPlane);
    XMStoreFloat4x4(&mReflectedSkullRitem->World, skullWorld * R);

    XMVECTOR shadowPlane = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR toMainLight = -XMLoadFloat3(&mMainPassCB.Lights[0].Direction);
    XMMATRIX S = XMMatrixShadow(shadowPlane, toMainLight);
    XMMATRIX shadowOffset = XMMatrixTranslation(0.0f, 0.001f, 0.0f);
    XMStoreFloat4x4(&mShadowedSkullRitem->World, skullWorld * S * shadowOffset);

    mSkullRitem->NumFramesDirty = gNumFrameResources;
    mReflectedSkullRitem->NumFramesDirty = gNumFrameResources;
    mShadowedSkullRitem->NumFramesDirty = gNumFrameResources;
}

void StencilApp::UpdateCamera(const GameTimer<float>& gt)
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

void StencilApp::AnimateMaterials(const GameTimer<float>& gt)
{

}

void StencilApp::UpdateObjectCBs(const GameTimer<float>& gt)
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

void StencilApp::UpdateMaterialCBs(const GameTimer<float>& gt)
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

void StencilApp::UpdateMainPassCB(const GameTimer<float>& gt)
{
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

    mMainPassCB.FogStart = 50.0f;
    mMainPassCB.FogRange = 100.0f;
    mMainPassCB.FogColor = { 0.7f, 0.7f, 0.7f, 1.0f };

    mMainPassCB.AmbientLight = { 0.1f, 0.1f, 0.1f, 1.0f };

    mMainPassCB.Lights[0].Type = (int)DirectXHelper::LIGHT_TYPE_DIRECTIONAL;
    XMVECTOR lightDir = -DirectXHelper::Math::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);
    XMStoreFloat3(&mMainPassCB.Lights[0].Direction, lightDir);
    mMainPassCB.Lights[0].Strength = { 0.9f, 0.9f, 0.9f };

    mMainPassCB.Lights[1].Type = (int)DirectXHelper::LIGHT_TYPE_POINT;
    mMainPassCB.Lights[1].Strength = { 0.1f, 0.9f, 0.1f };
    mMainPassCB.Lights[1].FalloffStart = 3.0f;
    mMainPassCB.Lights[1].FalloffEnd = 10.0f;
    mMainPassCB.Lights[1].Position = { 0.0f, 8.0f, 0.5f };

    mMainPassCB.Lights[2].Type = (int)DirectXHelper::LIGHT_TYPE_SPOTLIGHT;
    mMainPassCB.Lights[2].Strength = { 1.0f, 0.0f, 0.2f };
    mMainPassCB.Lights[2].FalloffStart = 0.5f;
    mMainPassCB.Lights[2].FalloffEnd = 20.0f;
    lightDir = XMVectorSet(0.0f, 0.0, 1.0f, 1.0f);
    lightDir = XMVector3Normalize(lightDir);
    XMStoreFloat3(&mMainPassCB.Lights[2].Direction, lightDir);
    mMainPassCB.Lights[2].Position = { 0.0f, 3.5f, -10.0f };
    mMainPassCB.Lights[2].SpotPower = 15.0f;

    mMainPassCB.NumLights = 3;

    DirectXHelper::UploadBuffer<PassConstants>* currPassCB = mCurrFrameResource->PassCB.get();
    currPassCB->CopyData(0, mMainPassCB);
}

void StencilApp::UpdateReflectedPassCB(const GameTimer<float>& gt)
{
    mReflectedPassCB = mMainPassCB;

    XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    XMMATRIX R = XMMatrixReflect(mirrorPlane);

    for(int i = 0; i < mMainPassCB.NumLights; i++)
    {
        XMVECTOR lightDir = XMLoadFloat3(&mMainPassCB.Lights[i].Direction);
        XMVECTOR reflectedLightDir = XMVector3TransformNormal(lightDir, R);

        XMVECTOR lightPos = XMLoadFloat3(&mMainPassCB.Lights[i].Position);
        XMVECTOR reflectedLightPos = XMVector3Transform(lightPos, R);

        XMStoreFloat3(&mReflectedPassCB.Lights[i].Direction, reflectedLightDir);
        XMStoreFloat3(&mReflectedPassCB.Lights[i].Position, reflectedLightPos);
    }

    auto currPassCB = mCurrFrameResource->PassCB.get();
    currPassCB->CopyData(1, mReflectedPassCB);
}

void StencilApp::LoadTextures()
{
    DDS_TEXTURE_FILE_INFO textureFiles[] = {
        L"bricksTex", L"Textures/bricks3.dds",
        L"checkboardTex", L"Textures/checkboard.dds",
        L"iceTex", L"Textures/ice.dds",
        L"white1x1Tex", L"Textures/white1x1.dds"
    };
    std::vector<DDS_TEXTURE> textures;
    textures.resize(ARRAYSIZE(textureFiles));

    ComPtr<IDStorageFactory> factory;
    ThrowIfFailed(DStorageGetFactory(IID_PPV_ARGS(factory.GetAddressOf())));

    DSTORAGE_QUEUE_DESC queueDesc = {};
    queueDesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;
    queueDesc.Priority = DSTORAGE_PRIORITY_HIGH;
    queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
    queueDesc.Device = md3dDevice.Get();

    ComPtr<IDStorageQueue> queue;
    ThrowIfFailed(factory->CreateQueue(&queueDesc, IID_PPV_ARGS(queue.GetAddressOf())));

    ThrowIfFailed(CreateDDSTexturesFromFileDStorage(
        md3dDevice.Get(),
        mCommandList.Get(),
        factory.Get(),
        queue.Get(),
        ARRAYSIZE(textureFiles),
        textureFiles,
        textures.data()
    ));

    for(auto& texture : textures)
    {
        AddTexture(texture);
    }
}

void StencilApp::BuildRootSignature()
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

void StencilApp::BuildDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = mMaxSrvHeapSize;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(mSrvDescriptorHeap.GetAddressOf())));

    CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    for(auto& tex : mTextures)
    {
        srvDesc.Format = tex.second->Resource->GetDesc().Format;
        srvDesc.Texture2D.MipLevels = tex.second->Resource->GetDesc().MipLevels;

        hDescriptor.InitOffsetted(
            mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            tex.second->SrvHeapIndex,
            mCbvSrvDescriptorSize
        );

        md3dDevice->CreateShaderResourceView(tex.second->Resource.Get(), &srvDesc, hDescriptor);
    }

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

void StencilApp::BuildShadersAndInputLayout()
{
    const D3D_SHADER_MACRO alphaTestDefines[] = {
        "ALPHA_TEST", "1",
        nullptr, nullptr
    };

    const D3D_SHADER_MACRO opaqueDefines[] = {
        nullptr, nullptr
    };

    mShaders[L"standardVS"] = DirectXHelper::CompileShader(
        L"Shaders\\TexLighting.hlsl",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "VS",
        "vs_5_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0
    );

    mShaders[L"opaquePS"] = DirectXHelper::CompileShader(
        L"Shaders\\TexLighting.hlsl",
        opaqueDefines,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "PS",
        "ps_5_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0
    );

    mShaders[L"alphaTestedPS"] = DirectXHelper::CompileShader(
        L"Shaders\\TexLighting.hlsl",
        alphaTestDefines,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "PS",
        "ps_5_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0
    );

    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
}

void StencilApp::BuildGeometry()
{
    {
        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> indices;

        ComPtr<IDStorageFactory> factory;
        ThrowIfFailed(DStorageGetFactory(IID_PPV_ARGS(factory.GetAddressOf())));

        ComPtr<IDStorageFile> file;
        ThrowIfFailed(factory->OpenFile(L"Models\\skull.txt", IID_PPV_ARGS(file.GetAddressOf())));

        BY_HANDLE_FILE_INFORMATION info = {};
        ThrowIfFailed(file->GetFileInformation(&info));
        std::uint32_t fileSize = info.nFileSizeLow;

        DSTORAGE_QUEUE_DESC queueDesc = {};
        queueDesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;
        queueDesc.Priority = DSTORAGE_PRIORITY_HIGH;
        queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
        queueDesc.Device = md3dDevice.Get();

        ComPtr<IDStorageQueue> queue;
        ThrowIfFailed(factory->CreateQueue(&queueDesc, IID_PPV_ARGS(queue.GetAddressOf())));

        std::vector<char> fileContent;
        fileContent.resize(fileSize);

        DSTORAGE_REQUEST request = {};
        request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
        request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_MEMORY;
        request.Source.File.Source = file.Get();
        request.Source.File.Offset = 0;
        request.Source.File.Size = fileSize;
        request.UncompressedSize = fileSize;
        request.Destination.Memory.Buffer = (void*)fileContent.data();
        request.Destination.Memory.Size = fileSize;

        queue->EnqueueRequest(&request);

        ComPtr<ID3D12Fence> fence;
        ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf())));

        HANDLE eventHandle = CreateEventExW(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        constexpr uint64_t fenceValue = 1;
        ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, eventHandle));
        queue->EnqueueSignal(fence.Get(), fenceValue);

        queue->Submit();

        WaitForSingleObject(eventHandle, INFINITE);

        DSTORAGE_ERROR_RECORD errorRecord = {};
        queue->RetrieveErrorRecord(&errorRecord);

        ThrowIfFailed(errorRecord.FirstFailure.HResult);

        std::istringstream fin(fileContent.data(), std::ios_base::in);
        if(true)
        {
            std::string s;
            std::getline(fin, s, ' ');
            std::getline(fin, s);
            int vertCount = std::stoi(s);

            std::getline(fin, s, ' ');
            std::getline(fin, s);
            int triCount = std::stoi(s);

            std::getline(fin, s);
            std::getline(fin, s);

            vertices.resize(vertCount);
            indices.resize(triCount * 3);

            for(int i = 0; i < vertCount; i++)
            {
                std::getline(fin, s, ' ');
                float x = std::stof(s);
                std::getline(fin, s, ' ');
                float y = std::stof(s);
                std::getline(fin, s, ' ');
                float z = std::stof(s);
                std::getline(fin, s, ' ');
                float r = std::stof(s);
                std::getline(fin, s, ' ');
                float g = std::stof(s);
                std::getline(fin, s);
                float b = std::stof(s);

                vertices[i].Pos = float3(x, y, z);
                vertices[i].Normal = float3(r, g, b);
                vertices[i].TexC = float2(0.0f, 0.0f);
            }

            std::getline(fin, s);
            std::getline(fin, s);
            std::getline(fin, s);

            for(int i = 0; i < triCount; i++)
            {
                std::getline(fin, s, ' ');
                indices[i * 3] = std::stoi(s);
                std::getline(fin, s, ' ');
                indices[i * 3 + 1] = std::stoi(s);
                std::getline(fin, s);
                indices[i * 3 + 2] = std::stoi(s);
            }
        }

        UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
        UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint32_t);

        std::unique_ptr<DirectXHelper::MeshGeometry> geo = std::make_unique<DirectXHelper::MeshGeometry>();
        geo->Name = L"skullGeo";

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
        geo->IndexFormat = DXGI_FORMAT_R32_UINT;
        geo->IndexBufferByteSize = ibByteSize;

        DirectXHelper::MeshGeometry::SubmeshGeometry submesh = {};
        submesh.StartIndexLocation = 0;
        submesh.IndexCount = (UINT)indices.size();
        submesh.BaseVertexLocation = 0;

        geo->DrawArgs[L"skull"] = submesh;

        mGeometries[geo->Name] = std::move(geo);
    }

    {
        std::array<Vertex, 20> vertices =
        {
            // Floor: Observe we tile texture coordinates.
            Vertex(-3.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f), // 0 
            Vertex(-3.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
            Vertex(7.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f),
            Vertex(7.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 4.0f, 4.0f),

            // Wall: Observe we tile texture coordinates, and that we
            // leave a gap in the middle for the mirror.
            Vertex(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 4
            Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
            Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f),
            Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 2.0f),

            Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 8 
            Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
            Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f),
            Vertex(7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 2.0f),

            Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 12
            Vertex(-3.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
            Vertex(7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f),
            Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 1.0f),

            // Mirror
            Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 16
            Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
            Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f),
            Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f)
        };

        std::array<std::int16_t, 30> indices =
        {
            // Floor
            0, 1, 2,
            0, 2, 3,

            // Walls
            4, 5, 6,
            4, 6, 7,

            8, 9, 10,
            8, 10, 11,

            12, 13, 14,
            12, 14, 15,

            // Mirror
            16, 17, 18,
            16, 18, 19
        };

        DirectXHelper::MeshGeometry::SubmeshGeometry floorSubmesh;
        floorSubmesh.IndexCount = 6;
        floorSubmesh.StartIndexLocation = 0;
        floorSubmesh.BaseVertexLocation = 0;

        DirectXHelper::MeshGeometry::SubmeshGeometry wallSubmesh;
        wallSubmesh.IndexCount = 18;
        wallSubmesh.StartIndexLocation = 6;
        wallSubmesh.BaseVertexLocation = 0;

        DirectXHelper::MeshGeometry::SubmeshGeometry mirrorSubmesh;
        mirrorSubmesh.IndexCount = 6;
        mirrorSubmesh.StartIndexLocation = 24;
        mirrorSubmesh.BaseVertexLocation = 0;

        const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
        const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

        auto geo = std::make_unique<DirectXHelper::MeshGeometry>();
        geo->Name = L"roomGeo";

        ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
        CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

        ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
        CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

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

        geo->DrawArgs[L"floor"] = floorSubmesh;
        geo->DrawArgs[L"wall"] = wallSubmesh;
        geo->DrawArgs[L"mirror"] = mirrorSubmesh;

        mGeometries[geo->Name] = std::move(geo);
    }
}

void StencilApp::BuildPSOs()
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

    D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = opaquePsoDesc;
    alphaTestedPsoDesc.PS.pShaderBytecode = (BYTE*)mShaders[L"alphaTestedPS"]->GetBufferPointer();
    alphaTestedPsoDesc.PS.BytecodeLength = mShaders[L"alphaTestedPS"]->GetBufferSize();
    alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(mPSOs[L"alphaTested"].GetAddressOf())));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;
    D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc = {};
    transparencyBlendDesc.BlendEnable = true;
    transparencyBlendDesc.LogicOpEnable = false;
    transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
    transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
    transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
    transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
    transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
    transparentPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(mPSOs[L"transparent"].GetAddressOf())));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC markMirrorsPsoDesc = opaquePsoDesc;
    CD3DX12_BLEND_DESC mirrorBlendState(D3D12_DEFAULT);
    mirrorBlendState.RenderTarget[0].RenderTargetWriteMask = 0;

    D3D12_DEPTH_STENCIL_DESC mirrorDSS = {};
    mirrorDSS.DepthEnable = true;
    mirrorDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    mirrorDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    mirrorDSS.StencilEnable = true;
    mirrorDSS.StencilReadMask = 0xff;
    mirrorDSS.StencilWriteMask = 0xff;
    mirrorDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    mirrorDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    mirrorDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
    mirrorDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    mirrorDSS.BackFace = mirrorDSS.FrontFace;

    markMirrorsPsoDesc.BlendState = mirrorBlendState;
    markMirrorsPsoDesc.DepthStencilState = mirrorDSS;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&markMirrorsPsoDesc, IID_PPV_ARGS(mPSOs[L"markStencilMirrors"].GetAddressOf())));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC drawOpaqueReflectionsPsoDesc = opaquePsoDesc;
    D3D12_DEPTH_STENCIL_DESC reflectionsDSS = {};
    reflectionsDSS.DepthEnable = true;
    reflectionsDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    reflectionsDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    reflectionsDSS.StencilEnable = true;
    reflectionsDSS.StencilReadMask = 0xff;
    reflectionsDSS.StencilWriteMask = 0xff;
    reflectionsDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    reflectionsDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    reflectionsDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    reflectionsDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
    reflectionsDSS.BackFace = reflectionsDSS.FrontFace;

    drawOpaqueReflectionsPsoDesc.DepthStencilState = reflectionsDSS;
    drawOpaqueReflectionsPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    drawOpaqueReflectionsPsoDesc.RasterizerState.FrontCounterClockwise = true;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&drawOpaqueReflectionsPsoDesc, IID_PPV_ARGS(mPSOs[L"drawOpaqueStencilReflections"].GetAddressOf())));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC drawAlphaTestedReflectionsPsoDesc = alphaTestedPsoDesc;
    drawAlphaTestedReflectionsPsoDesc.DepthStencilState = reflectionsDSS;
    drawAlphaTestedReflectionsPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    drawAlphaTestedReflectionsPsoDesc.RasterizerState.FrontCounterClockwise = true;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&drawAlphaTestedReflectionsPsoDesc, IID_PPV_ARGS(mPSOs[L"drawAlphaTestedStencilReflections"].GetAddressOf())));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowPsoDesc = transparentPsoDesc;
    D3D12_DEPTH_STENCIL_DESC shadowDSS = {};
    shadowDSS.DepthEnable = true;
    shadowDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    shadowDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    shadowDSS.StencilEnable = true;
    shadowDSS.StencilReadMask = 0xff;
    shadowDSS.StencilWriteMask = 0xff;
    shadowDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    shadowDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    shadowDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
    shadowDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
    shadowDSS.BackFace = reflectionsDSS.FrontFace;

    shadowPsoDesc.DepthStencilState = shadowDSS;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&shadowPsoDesc, IID_PPV_ARGS(mPSOs[L"shadow"].GetAddressOf())));
}

void StencilApp::BuildFrameResources()
{
    for(int i = 0; i < gNumFrameResources; i++)
    {
        mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(), 2, (UINT)mMaxRenderItemNumber, (UINT)mMaxMaterialNumber));
    }
}

void StencilApp::BuildMaterials()
{
    DirectXHelper::MaterialConstants matConst;

    matConst.DiffuseAlbedo = float4(1.0f, 1.0f, 1.0f, 1.0f);
    matConst.FresnelR0 = float3(0.05f, 0.05f, 0.05f);
    matConst.Roughness = 0.8f;
    AddMaterial(L"bricks", L"bricksTex", matConst);

    matConst.DiffuseAlbedo = float4(1.0f, 1.0f, 1.0f, 1.0f);
    matConst.FresnelR0 = float3(0.07f, 0.07f, 0.07f);
    matConst.Roughness = 0.3f;
    AddMaterial(L"checkerboard", L"checkboardTex", matConst);

    matConst.DiffuseAlbedo = float4(1.0f, 1.0f, 1.0f, 0.3f);
    matConst.FresnelR0 = float3(0.1f, 0.1f, 0.1f);
    matConst.Roughness = 0.05f;
    AddMaterial(L"icemirror", L"iceTex", matConst);

    matConst.DiffuseAlbedo = float4(1.0f, 1.0f, 1.0f, 1.0f);
    matConst.FresnelR0 = float3(0.05f, 0.05f, 0.05f);
    matConst.Roughness = 0.3f;
    AddMaterial(L"skullMat", L"white1x1Tex", matConst);

    matConst.DiffuseAlbedo = float4(0.0f, 0.0f, 0.0f, 0.5f);
    matConst.FresnelR0 = float3(0.001f, 0.001f, 0.001f);
    matConst.Roughness = 1.0f;
    AddMaterial(L"shadowMat", L"white1x1Tex", matConst);
}

void StencilApp::BuildRenderItems()
{
    float4x4 world = DirectXHelper::Math::Identity4X4();
    float4x4 texTransform = DirectXHelper::Math::Identity4X4();

    AddRenderItem(
        RenderLayer::Opaque,
        L"roomGeo",
        L"floor",
        L"checkerboard",
        world,
        texTransform
    );

    float4x4 mirrorReflect = {};
    XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    XMMATRIX R = XMMatrixReflect(mirrorPlane);
    XMStoreFloat4x4(&mirrorReflect, R);

    AddRenderItem(
        RenderLayer::OpaqueReflected,
        L"roomGeo",
        L"floor",
        L"checkerboard",
        mirrorReflect,
        texTransform
    );

    AddRenderItem(
        RenderLayer::Opaque,
        L"roomGeo",
        L"wall",
        L"bricks",
        world,
        texTransform
    );

    mSkullRitem = AddRenderItem(
        RenderLayer::Opaque,
        L"skullGeo",
        L"skull",
        L"skullMat",
        world,
        texTransform
    );

    mReflectedSkullRitem = AddRenderItem(
        RenderLayer::OpaqueReflected,
        L"skullGeo",
        L"skull",
        L"skullMat",
        world,
        texTransform
    );

    mShadowedSkullRitem = AddRenderItem(
        RenderLayer::Shadow,
        L"skullGeo",
        L"skull",
        L"shadowMat",
        world,
        texTransform
    );

    AddRenderItem(
        RenderLayer::Mirrors,
        L"roomGeo",
        L"mirror",
        L"icemirror",
        world,
        texTransform
    );

    AddRenderItem(
        RenderLayer::Transparent,
        L"roomGeo",
        L"mirror",
        L"icemirror",
        world,
        texTransform
    );

    /*
    std::unique_ptr<RenderItem> floorRitem = std::make_unique<RenderItem>();
    floorRitem->World = DirectXHelper::Math::Identity4X4();
    floorRitem->TexTransform = DirectXHelper::Math::Identity4X4();
    floorRitem->ObjCBIndex = 0;
    floorRitem->Mat = mMaterials[L"checkerboard"].get();
    floorRitem->Geo = mGeometries[L"roomGeo"].get();
    floorRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    floorRitem->IndexCount = floorRitem->Geo->DrawArgs[L"floor"].IndexCount;
    floorRitem->StartIndexLocation = floorRitem->Geo->DrawArgs[L"floor"].StartIndexLocation;
    floorRitem->BaseVertexLocation = floorRitem->Geo->DrawArgs[L"floor"].BaseVertexLocation;

    mRItemLayer[(int)RenderLayer::Opaque].push_back(floorRitem.get());

    std::unique_ptr<RenderItem> reflectedFloorRitem = std::make_unique<RenderItem>();
    *reflectedFloorRitem = *floorRitem;
    XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    XMMATRIX R = XMMatrixReflect(mirrorPlane);
    XMStoreFloat4x4(&reflectedFloorRitem->World, R);
    reflectedFloorRitem->ObjCBIndex = 1;

    mRItemLayer[(int)RenderLayer::OpaqueReflected].push_back(reflectedFloorRitem.get());

    std::unique_ptr<RenderItem> wallsRitem = std::make_unique<RenderItem>();
    wallsRitem->World = DirectXHelper::Math::Identity4X4();
    wallsRitem->TexTransform = DirectXHelper::Math::Identity4X4();
    wallsRitem->ObjCBIndex = 2;
    wallsRitem->Mat = mMaterials[L"bricks"].get();
    wallsRitem->Geo = mGeometries[L"roomGeo"].get();
    wallsRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    wallsRitem->IndexCount = wallsRitem->Geo->DrawArgs[L"wall"].IndexCount;
    wallsRitem->StartIndexLocation = wallsRitem->Geo->DrawArgs[L"wall"].StartIndexLocation;
    wallsRitem->BaseVertexLocation = wallsRitem->Geo->DrawArgs[L"wall"].BaseVertexLocation;

    mRItemLayer[(int)RenderLayer::Opaque].push_back(wallsRitem.get());

    std::unique_ptr<RenderItem> skullRitem = std::make_unique<RenderItem>();
    skullRitem->World = DirectXHelper::Math::Identity4X4();
    skullRitem->TexTransform = DirectXHelper::Math::Identity4X4();
    skullRitem->ObjCBIndex = 3;
    skullRitem->Mat = mMaterials[L"skullMat"].get();
    skullRitem->Geo = mGeometries[L"skullGeo"].get();
    skullRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    skullRitem->IndexCount = skullRitem->Geo->DrawArgs[L"skull"].IndexCount;
    skullRitem->StartIndexLocation = skullRitem->Geo->DrawArgs[L"skull"].StartIndexLocation;
    skullRitem->BaseVertexLocation = skullRitem->Geo->DrawArgs[L"skull"].BaseVertexLocation;
    mSkullRitem = skullRitem.get();

    mRItemLayer[(int)RenderLayer::Opaque].push_back(skullRitem.get());

    std::unique_ptr<RenderItem> reflectedSkullRitem = std::make_unique<RenderItem>();
    *reflectedSkullRitem = *skullRitem;
    reflectedSkullRitem->ObjCBIndex = 4;
    mReflectedSkullRitem = reflectedSkullRitem.get();

    mRItemLayer[(int)RenderLayer::OpaqueReflected].push_back(reflectedSkullRitem.get());

    std::unique_ptr<RenderItem> shadowSkullRitem = std::make_unique<RenderItem>();
    *shadowSkullRitem = *skullRitem;
    shadowSkullRitem->ObjCBIndex = 5;
    shadowSkullRitem->Mat = mMaterials[L"shadowMat"].get();
    mShadowedSkullRitem = shadowSkullRitem.get();
    
    mRItemLayer[(int)RenderLayer::Shadow].push_back(shadowSkullRitem.get());

    std::unique_ptr<RenderItem> mirrorRitem = std::make_unique<RenderItem>();
    mirrorRitem->World = DirectXHelper::Math::Identity4X4();
    mirrorRitem->TexTransform = DirectXHelper::Math::Identity4X4();
    mirrorRitem->ObjCBIndex = 6;
    mirrorRitem->Mat = mMaterials[L"icemirror"].get();
    mirrorRitem->Geo = mGeometries[L"roomGeo"].get();
    mirrorRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    mirrorRitem->IndexCount = mirrorRitem->Geo->DrawArgs[L"mirror"].IndexCount;
    mirrorRitem->StartIndexLocation = mirrorRitem->Geo->DrawArgs[L"mirror"].StartIndexLocation;
    mirrorRitem->BaseVertexLocation = mirrorRitem->Geo->DrawArgs[L"mirror"].BaseVertexLocation;

    mRItemLayer[(int)RenderLayer::Mirrors].push_back(mirrorRitem.get());
    mRItemLayer[(int)RenderLayer::Transparent].push_back(mirrorRitem.get());

    mAllRItems.push_back(std::move(floorRitem));
    mAllRItems.push_back(std::move(reflectedFloorRitem));
    mAllRItems.push_back(std::move(wallsRitem));
    mAllRItems.push_back(std::move(skullRitem));
    mAllRItems.push_back(std::move(reflectedSkullRitem));
    mAllRItems.push_back(std::move(shadowSkullRitem));
    mAllRItems.push_back(std::move(mirrorRitem));
    */
}

void StencilApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& rItems)
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

void StencilApp::AddTexture(DDS_TEXTURE& texture)
{
    auto texData = std::make_unique<DirectXHelper::Texture>();
    texData->Name = texture.TextureFile.TextureName;
    texData->Filename = texture.TextureFile.TextureFileUrl;
    texData->SrvHeapIndex = mMaxSrvHeapSize;
    texData->Resource = texture.Texture;
    texData->UploadHeap = texture.TextureUploadHeap;

    mMaxSrvHeapSize++;

    mTextures[texData->Name] = std::move(texData);
}

void StencilApp::AddMaterial(LPCWSTR name, LPCWSTR diffuseTexture, DirectXHelper::MaterialConstants& matConst)
{
    std::unique_ptr<DirectXHelper::Material> mat = std::make_unique<DirectXHelper::Material>();
    mat->Name = name;
    mat->MatCBIndex = mMaxMaterialNumber;
    mat->DiffuseSrvHeapIndex = mTextures[diffuseTexture]->SrvHeapIndex;
    mat->NumFramesDirty = gNumFrameResources;
    mat->DiffuseAlbedo = matConst.DiffuseAlbedo;
    mat->FresnelR0 = matConst.FresnelR0;
    mat->Roughness = matConst.Roughness;
    mat->MatTransform = matConst.MatTransform;

    mMaxMaterialNumber++;

    mMaterials[mat->Name] = std::move(mat);
}

RenderItem* StencilApp::AddRenderItem(RenderLayer layer, LPCWSTR geometry, LPCWSTR subgeometry, LPCWSTR material, float4x4& worldTransform, float4x4& textureTransform, D3D12_PRIMITIVE_TOPOLOGY primitiveType)
{
    std::unique_ptr<RenderItem> renderItem = std::make_unique<RenderItem>();
    renderItem->World = worldTransform;
    renderItem->TexTransform = textureTransform;
    renderItem->ObjCBIndex = mMaxRenderItemNumber;
    renderItem->Mat = mMaterials[material].get();
    renderItem->Geo = mGeometries[geometry].get();
    renderItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    renderItem->IndexCount = renderItem->Geo->DrawArgs[subgeometry].IndexCount;
    renderItem->StartIndexLocation = renderItem->Geo->DrawArgs[subgeometry].StartIndexLocation;
    renderItem->BaseVertexLocation = renderItem->Geo->DrawArgs[subgeometry].BaseVertexLocation;

    mMaxRenderItemNumber++;

    mRItemLayer[(int)layer].push_back(renderItem.get());
    RenderItem* ret = renderItem.get();
    mAllRItems.push_back(std::move(renderItem));

    return ret;
}

#endif