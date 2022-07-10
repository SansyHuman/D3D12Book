#pragma once

#ifndef HLSLTYPE_USE_MATRICES
#define HLSLTYPE_USE_MATRICES
#endif

#include "Common/framework.h"
#include "Common/d3dApp.h"
#include "Common/GeometryGenerator.h"
#include <ppl.h>

#ifndef D3D12BOOK_LANDANDWAVESAPP_H
#define D3D12BOOK_LANDANDWAVESAPP_H

using namespace DirectX;
using namespace DirectX::PackedVector;

struct Vertex
{
    float3 Pos;
    float4 Color;
};

cbuffer ObjectConstants _register(b0)
{
    float4x4 World = DirectXHelper::Math::Identity4X4();
};

cbuffer PassConstants _register(b1)
{
    float4x4 View = DirectXHelper::Math::Identity4X4();
    float4x4 InvView = DirectXHelper::Math::Identity4X4();
    float4x4 Proj = DirectXHelper::Math::Identity4X4();
    float4x4 InvProj = DirectXHelper::Math::Identity4X4();
    float4x4 ViewProj = DirectXHelper::Math::Identity4X4();
    float4x4 InvViewProj = DirectXHelper::Math::Identity4X4();
    float3 EyePosW = { 0.0f, 0.0f, 0.0f };
    float cbPerObjectPad1 = 0.0f;
    float2 RenderTargetSize = { 0.0f, 0.0f };
    float2 InvRenderTargetSize = { 0.0f, 0.0f };
    float NearZ = 0.0f;
    float FarZ = 0.0f;
    float TotalTime = 0.0f;
    float DeltaTime = 0.0f;
    float UnscaledTotalTime = 0.0f;
    float UnscaledDeltaTime = 0.0f;
    float2 cbPerObjectPad2 = float2(0.0f, 0.0f);
};

struct FrameResource
{
public:
    ComPtr<ID3D12CommandAllocator> CmdListAlloc;

    std::unique_ptr<DirectXHelper::UploadBuffer<PassConstants>> PassCB = nullptr;
    std::unique_ptr<DirectXHelper::UploadBuffer<ObjectConstants>> ObjectCB = nullptr;

    std::unique_ptr<DirectXHelper::UploadBuffer<Vertex>> WavesVB = nullptr;

    UINT64 Fence = 0;

    FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT waveVertCount)
    {
        ThrowIfFailed(device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(CmdListAlloc.GetAddressOf())
        ));

        PassCB = std::make_unique<DirectXHelper::UploadBuffer<PassConstants>>(device, passCount, true);
        ObjectCB = std::make_unique<DirectXHelper::UploadBuffer<ObjectConstants>>(device, objectCount, true);
        WavesVB = std::make_unique<DirectXHelper::UploadBuffer<Vertex>>(device, waveVertCount, false);
    }

    FrameResource(const FrameResource&) = delete;
    FrameResource& operator=(const FrameResource&) = delete;
    ~FrameResource() {}
};

constexpr int gNumFrameResources = 3;

struct RenderItem
{
    float4x4 World = DirectXHelper::Math::Identity4X4();
    int NumFramesDirty = gNumFrameResources;
    UINT ObjCBIndex = -1;
    DirectXHelper::MeshGeometry* Geo = nullptr;
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    INT BaseVertexLocation = 0;

    RenderItem() = default;
};

class Waves
{
private:
    int mNumRows = 0;
    int mNumCols = 0;

    int mVertexCount = 0;
    int mTriangleCount = 0;

    float mSpeed = 0.0f;
    float mDamping = 0.0f;

    float mK1 = 0.0f;
    float mK2 = 0.0f;
    float mK3 = 0.0f;

    float mTimeStep = 0.0f;
    float mSpatialStep = 0.0f;

    std::vector<float3> mPrevSolution;
    std::vector<float3> mCurrSolution;
    std::vector<float3> mNormals;
    std::vector<float3> mTangentX;

public:
    Waves(int m, int n, float dx, float dt, float speed, float damping);
    Waves(const Waves&) = delete;
    Waves& operator=(const Waves&) = delete;
    ~Waves() {}

    int RowCount() const { return mNumRows; }
    int ColumnCount() const { return mNumCols; }
    int VertexCount() const { return mVertexCount; }
    int TriangleCount() const { return mTriangleCount; }
    float Width() const { return mNumCols * mSpatialStep; }
    float Depth() const { return mNumRows * mSpatialStep; }

    const float3& Position(int i) const { return mCurrSolution[i]; }
    const float3& Normal(int i) const { return mNormals[i]; }
    const float3& TangentX(int i) const { return mTangentX[i]; }

    void Update(float dt);
    void Disturb(int i, int j, float magnitude);
};

Waves::Waves(int m, int n, float dx, float dt, float speed, float damping)
{
    mNumRows = m;
    mNumCols = n;

    mVertexCount = m * n;
    mTriangleCount = (m - 1) * (n - 1) * 2;

    mTimeStep = dt;
    mSpatialStep = dx;

    mSpeed = speed;
    mDamping = damping;

    float d = damping * dt + 2.0f;
    float e = speed * speed * dt * dt / (dx * dx);
    mK1 = (damping * dt - 2.0f) / d;
    mK2 = (4.0f - 8.0f * e) / d;
    mK3 = (2.0f * e) / d;

    mPrevSolution.resize(m * n);
    mCurrSolution.resize(m * n);
    mNormals.resize(m * n);
    mTangentX.resize(m * n);

    float halfWidth = (n - 1) * dx * 0.5f;
    float halfDepth = (m - 1) * dx * 0.5f;
    for(int i = 0; i < m; i++)
    {
        float z = halfDepth - i * dx;
        for(int j = 0; j < n; j++)
        {
            float x = -halfWidth + j * dx;

            mPrevSolution[i * n + j] = float3(x, 0.0f, z);
            mCurrSolution[i * n + j] = float3(x, 0.0f, z);
            mNormals[i * n + j] = float3(0, 1, 0);
            mTangentX[i * n + j] = float3(1, 0, 0);
        }
    }
}

void Waves::Update(float dt)
{
    static float t = 0;

    if(abs(dt) < 0.0001f)
        return;

    // Accumulate time.
    t += dt;

    // Only update the simulation at the specified time step.
    if(t >= mTimeStep)
    {
        Start:
        // Only update interior points; we use zero boundary conditions.
        concurrency::parallel_for(1, mNumRows - 1, [this](int i)
                                  //for(int i = 1; i < mNumRows-1; ++i)
                                  {
                                      for(int j = 1; j < mNumCols - 1; ++j)
                                      {
                                          // After this update we will be discarding the old previous
                                          // buffer, so overwrite that buffer with the new update.
                                          // Note how we can do this inplace (read/write to same element) 
                                          // because we won't need prev_ij again and the assignment happens last.

                                          // Note j indexes x and i indexes z: h(x_j, z_i, t_k)
                                          // Moreover, our +z axis goes "down"; this is just to 
                                          // keep consistent with our row indices going down.

                                          mPrevSolution[i * mNumCols + j].y =
                                              mK1 * mPrevSolution[i * mNumCols + j].y +
                                              mK2 * mCurrSolution[i * mNumCols + j].y +
                                              mK3 * (mCurrSolution[(i + 1) * mNumCols + j].y +
                                                     mCurrSolution[(i - 1) * mNumCols + j].y +
                                                     mCurrSolution[i * mNumCols + j + 1].y +
                                                     mCurrSolution[i * mNumCols + j - 1].y);
                                      }
                                  });

        // We just overwrote the previous buffer with the new data, so
        // this data needs to become the current solution and the old
        // current solution becomes the new previous solution.
        std::swap(mPrevSolution, mCurrSolution);

        t -= mTimeStep; // reset time
        if(t < 0.0f)
            t = 0.0f;

        //
        // Compute normals using finite difference scheme.
        //
        concurrency::parallel_for(1, mNumRows - 1, [this](int i)
                                  //for(int i = 1; i < mNumRows - 1; ++i)
                                  {
                                      for(int j = 1; j < mNumCols - 1; ++j)
                                      {
                                          float l = mCurrSolution[i * mNumCols + j - 1].y;
                                          float r = mCurrSolution[i * mNumCols + j + 1].y;
                                          float t = mCurrSolution[(i - 1) * mNumCols + j].y;
                                          float b = mCurrSolution[(i + 1) * mNumCols + j].y;
                                          mNormals[i * mNumCols + j].x = -r + l;
                                          mNormals[i * mNumCols + j].y = 2.0f * mSpatialStep;
                                          mNormals[i * mNumCols + j].z = b - t;

                                          XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&mNormals[i * mNumCols + j]));
                                          XMStoreFloat3(&mNormals[i * mNumCols + j], n);

                                          mTangentX[i * mNumCols + j] = XMFLOAT3(2.0f * mSpatialStep, r - l, 0.0f);
                                          XMVECTOR T = XMVector3Normalize(XMLoadFloat3(&mTangentX[i * mNumCols + j]));
                                          XMStoreFloat3(&mTangentX[i * mNumCols + j], T);
                                      }
                                  });

        if(t > 0.0f)
            goto Start;
    }
}

void Waves::Disturb(int i, int j, float magnitude)
{
    // Don't disturb boundaries.
    assert(i > 1 && i < mNumRows - 2);
    assert(j > 1 && j < mNumCols - 2);

    float halfMag = 0.5f * magnitude;

    // Disturb the ijth vertex height and its neighbors.
    mCurrSolution[i * mNumCols + j].y += magnitude;
    mCurrSolution[i * mNumCols + j + 1].y += halfMag;
    mCurrSolution[i * mNumCols + j - 1].y += halfMag;
    mCurrSolution[(i + 1) * mNumCols + j].y += halfMag;
    mCurrSolution[(i - 1) * mNumCols + j].y += halfMag;
}

enum class RenderLayer : int
{
    Opaque = 0,
    Count
};

class LandAndWavesApp : public D3DApp
{
private:
    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrFrameResource = nullptr;
    int mCurrFrameResourceIndex = 0;

    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

    std::unordered_map<std::wstring, std::unique_ptr<DirectXHelper::MeshGeometry>> mGeometries;
    std::unordered_map<std::wstring, ComPtr<ID3DBlob>> mShaders;
    std::unordered_map<std::wstring, ComPtr<ID3D12PipelineState>> mPSOs;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    RenderItem* mWavesRItem = nullptr;

    std::vector<std::unique_ptr<RenderItem>> mAllRItems;
    std::vector<RenderItem*> mRItemLayer[(int)RenderLayer::Count];

    std::unique_ptr<Waves> mWaves;

    PassConstants mMainPassCB;

    bool mIsWireframe = false;

    float3 mEyePos = { 0.0f, 0.0f, 0.0f };
    float4x4 mView = DirectXHelper::Math::Identity4X4();
    float4x4 mProj = DirectXHelper::Math::Identity4X4();

    float mPhi = 1.5f * XM_PI;
    float mTheta = XM_PIDIV4;
    float mRadius = 30.0f;

    float mSunTheta = 1.25f * XM_PI;
    float mSunPhi = XM_PIDIV4;

    POINT mLastMousePos;

public:
    LandAndWavesApp(HINSTANCE hInstance);
    LandAndWavesApp(const LandAndWavesApp&) = delete;
    LandAndWavesApp& operator=(const LandAndWavesApp&) = delete;
    ~LandAndWavesApp();

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
    void UpdateObjectCBs(const GameTimer<float>& gt);
    void UpdateMainPassCB(const GameTimer<float>& gt);
    void UpdateWaves(const GameTimer<float>& gt);

    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildLandGeometry();
    void BuildWavesGeometryBuffers();
    void BuildPSOs();
    void BuildFrameResources();
    void BuildRenderItems();
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& rItems);

    float GetHillsHeight(float x, float z) const;
    float3 GetHillsNormal(float x, float z) const;
};

LandAndWavesApp::LandAndWavesApp(HINSTANCE hInstance)
    : D3DApp(hInstance)
{
}

LandAndWavesApp::~LandAndWavesApp()
{
    if(md3dDevice != nullptr)
        FlushCommandQueue();
}

bool LandAndWavesApp::Initialize(const D3DApp::D3DAPP_SETTINGS& settings)
{
    if(!D3DApp::Initialize(settings))
        return false;

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    mWaves = std::make_unique<Waves>(320, 320, 0.5f, 0.016f, 5.0f, 0.4f);

    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildLandGeometry();
    BuildWavesGeometryBuffers();
    BuildRenderItems();
    BuildFrameResources();
    BuildPSOs();

    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdList[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(1, cmdList);

    FlushCommandQueue();

    return true;
}

LRESULT LandAndWavesApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

void LandAndWavesApp::OnResize()
{
    D3DApp::OnResize();

    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * XM_PI, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);
}

void LandAndWavesApp::Update(GameTimer<float>& gt)
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

    UpdateObjectCBs(gt);
    UpdateMainPassCB(gt);
    UpdateWaves(gt);
}

void LandAndWavesApp::Draw(const GameTimer<float>& gt)
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

    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    mCommandList->SetGraphicsRootConstantBufferView(1, mCurrFrameResource->PassCB->Resource()->GetGPUVirtualAddress());

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

void LandAndWavesApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;
    SetCapture(mhMainWnd);
}

void LandAndWavesApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void LandAndWavesApp::OnMouseMove(WPARAM btnState, int x, int y)
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

        mRadius = DirectXHelper::Math::Clamp<float>(mRadius, 5.0f, 230.0f);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

void LandAndWavesApp::OnKeyboardInput(const GameTimer<float>& gt)
{
    if(GetAsyncKeyState('1') & 0x8000)
        mIsWireframe = true;
    else
        mIsWireframe = false;
}

void LandAndWavesApp::UpdateCamera(const GameTimer<float>& gt)
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

void LandAndWavesApp::UpdateObjectCBs(const GameTimer<float>& gt)
{
    DirectXHelper::UploadBuffer<ObjectConstants>* currObjectCB = mCurrFrameResource->ObjectCB.get();

    for(auto& e : mAllRItems)
    {
        if(e->NumFramesDirty > 0)
        {
            XMMATRIX world = XMLoadFloat4x4(&e->World);

            ObjectConstants objConst = {};
            XMStoreFloat4x4(&objConst.World, XMMatrixTranspose(world));

            currObjectCB->CopyData(e->ObjCBIndex, objConst);

            e->NumFramesDirty--;
        }
    }
}

void LandAndWavesApp::UpdateMainPassCB(const GameTimer<float>& gt)
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
    mMainPassCB.RenderTargetSize = float2((float)mClientWidth, (float)mClientHeight);
    mMainPassCB.InvRenderTargetSize = float2(1.0f / mClientWidth, 1.0f / mClientHeight);
    mMainPassCB.NearZ = 1.0f;
    mMainPassCB.FarZ = 1000.0f;
    mMainPassCB.TotalTime = gt.TotalTime();
    mMainPassCB.UnscaledTotalTime = gt.UnscaledTotalTime();
    mMainPassCB.DeltaTime = gt.DeltaTime();
    mMainPassCB.UnscaledDeltaTime = gt.UnscaledDeltaTime();

    DirectXHelper::UploadBuffer<PassConstants>* currPassCB = mCurrFrameResource->PassCB.get();
    currPassCB->CopyData(0, mMainPassCB);
}

void LandAndWavesApp::UpdateWaves(const GameTimer<float>& gt)
{
    static float t_base = 0.0f;

    if((gt.TotalTime() - t_base) >= 0.1f)
    {
        t_base = gt.TotalTime();

        int i = DirectXHelper::Math::Rand(4, mWaves->RowCount() - 5);
        int j = DirectXHelper::Math::Rand(4, mWaves->ColumnCount() - 5);

        float r = DirectXHelper::Math::RandF(1.0f, 2.0f);

        mWaves->Disturb(i, j, r);
    }

    mWaves->Update(gt.DeltaTime());

    DirectXHelper::UploadBuffer<Vertex>* currWavesVB = mCurrFrameResource->WavesVB.get();
    for(int i = 0; i < mWaves->VertexCount(); i++)
    {
        Vertex v = {};

        v.Pos = mWaves->Position(i);
        v.Color = float4(DirectX::Colors::Blue);

        currWavesVB->CopyData(i, v);
    }

    mWavesRItem->Geo->VertexBufferGPU = currWavesVB->Resource();
}

void LandAndWavesApp::BuildRootSignature()
{
    CD3DX12_ROOT_PARAMETER slotRootParameter[2];

    slotRootParameter[0].InitAsConstantBufferView(0);
    slotRootParameter[1].InitAsConstantBufferView(1);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

void LandAndWavesApp::BuildShadersAndInputLayout()
{
    mShaders[L"standardVS"] = DirectXHelper::CompileShader(
        L"Shaders\\Shapes.hlsl",
        nullptr,
        nullptr,
        "VS",
        "vs_5_1",
        0, 0
    );

    mShaders[L"opaquePS"] = DirectXHelper::CompileShader(
        L"Shaders\\Shapes.hlsl",
        nullptr,
        nullptr,
        "PS",
        "ps_5_1",
        0, 0
    );


    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
}

void LandAndWavesApp::BuildLandGeometry()
{
    GeometryGenerator::MeshData<std::uint32_t> grid = GeometryGenerator::CreateGrid<std::uint32_t>(160.0f, 160.0f, 160, 160);
    
    std::vector<Vertex> vertices(grid.Vertices.size());
    for(size_t i = 0; i < grid.Vertices.size(); i++)
    {
        float3& p = grid.Vertices[i].Position;

        vertices[i].Pos = p;
        vertices[i].Pos.y = GetHillsHeight(p.x, p.z);

        if(vertices[i].Pos.y < -10.0f)
        {
            vertices[i].Color = float4(1.0f, 0.96f, 0.62f, 1.0f);
        }
        else if(vertices[i].Pos.y < 5.0f)
        {
            vertices[i].Color = float4(0.48f, 0.77f, 0.46f, 1.0f);
        }
        else if(vertices[i].Pos.y < 12.0f)
        {
            vertices[i].Color = float4(0.1f, 0.48f, 0.19f, 1.0f);
        }
        else if(vertices[i].Pos.y < 20.0f)
        {
            vertices[i].Color = float4(0.45f, 0.39f, 0.34f, 1.0f);
        }
        else
        {
            vertices[i].Color = float4(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

    std::vector<std::uint32_t> indices = grid.GetIndices32();
    UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint32_t);
    
    std::unique_ptr<DirectXHelper::MeshGeometry> geo = std::make_unique<DirectXHelper::MeshGeometry>();
    geo->Name = L"landGeo";

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
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    geo->DrawArgs[L"grid"] = submesh;

    mGeometries[L"landGeo"] = std::move(geo);
}

void LandAndWavesApp::BuildWavesGeometryBuffers()
{
    std::vector<std::uint32_t> indices(3 * mWaves->TriangleCount());

    int m = mWaves->RowCount();
    int n = mWaves->ColumnCount();
    int k = 0;
    for(int i = 0; i < m - 1; i++)
    {
        for(int j = 0; j < n - 1; j++)
        {
            indices[k] = i * n + j;
            indices[k + 1] = i * n + j + 1;
            indices[k + 2] = (i + 1) * n + j;

            indices[k + 3] = (i + 1) * n + j;
            indices[k + 4] = i * n + j + 1;
            indices[k + 5] = (i + 1) * n + j + 1;

            k += 6;
        }
    }

    UINT vbByteSize = mWaves->VertexCount() * sizeof(Vertex);
    UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint32_t);

    std::unique_ptr<DirectXHelper::MeshGeometry> geo = std::make_unique<DirectXHelper::MeshGeometry>();
    geo->Name = L"waterGeo";

    geo->VertexBufferCPU = nullptr;
    geo->VertexBufferGPU = nullptr;

    ThrowIfFailed(D3DCreateBlob(ibByteSize, geo->IndexBufferCPU.GetAddressOf()));
    memcpy_s(geo->IndexBufferCPU->GetBufferPointer(), ibByteSize, indices.data(), ibByteSize);

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
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    geo->DrawArgs[L"grid"] = submesh;

    mGeometries[L"waterGeo"] = std::move(geo);
}

void LandAndWavesApp::BuildPSOs()
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

void LandAndWavesApp::BuildFrameResources()
{
    for(int i = 0; i < gNumFrameResources; i++)
    {
        mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(), 1, (UINT)mAllRItems.size(), mWaves->VertexCount()));
    }
}

void LandAndWavesApp::BuildRenderItems()
{
    std::unique_ptr<RenderItem> waves = std::make_unique<RenderItem>();
    waves->World = DirectXHelper::Math::Identity4X4();
    waves->ObjCBIndex = 0;
    waves->Geo = mGeometries[L"waterGeo"].get();
    waves->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    waves->IndexCount = waves->Geo->DrawArgs[L"grid"].IndexCount;
    waves->StartIndexLocation = waves->Geo->DrawArgs[L"grid"].StartIndexLocation;
    waves->BaseVertexLocation = waves->Geo->DrawArgs[L"grid"].BaseVertexLocation;

    mWavesRItem = waves.get();
    mRItemLayer[(int)RenderLayer::Opaque].push_back(waves.get());

    std::unique_ptr<RenderItem> grid = std::make_unique<RenderItem>();
    grid->World = DirectXHelper::Math::Identity4X4();
    grid->ObjCBIndex = 1;
    grid->Geo = mGeometries[L"landGeo"].get();
    grid->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    grid->IndexCount = grid->Geo->DrawArgs[L"grid"].IndexCount;
    grid->StartIndexLocation = grid->Geo->DrawArgs[L"grid"].StartIndexLocation;
    grid->BaseVertexLocation = grid->Geo->DrawArgs[L"grid"].BaseVertexLocation;

    mRItemLayer[(int)RenderLayer::Opaque].push_back(grid.get());

    mAllRItems.push_back(std::move(waves));
    mAllRItems.push_back(std::move(grid));
}

void LandAndWavesApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& rItems)
{
    UINT objCBSize = DirectXHelper::CalcConstantBufferByteSize<ObjectConstants>();
    ID3D12Resource* objectCB = mCurrFrameResource->ObjectCB->Resource();
    
    for(size_t i = 0; i < rItems.size(); i++)
    {
        RenderItem* ri = rItems[i];

        D3D12_VERTEX_BUFFER_VIEW vbv = ri->Geo->VertexBufferView();
        D3D12_INDEX_BUFFER_VIEW ibv = ri->Geo->IndexBufferView();
        cmdList->IASetVertexBuffers(0, 1, &vbv);
        cmdList->IASetIndexBuffer(&ibv);
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

        cmdList->SetGraphicsRootConstantBufferView(0, objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBSize);

        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }
}

float LandAndWavesApp::GetHillsHeight(float x, float z) const
{
    return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

float3 LandAndWavesApp::GetHillsNormal(float x, float z) const
{
    float3 n(
        -0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
        1.0f,
        -0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));

    XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
    XMStoreFloat3(&n, unitNormal);

    return n;
}

#endif