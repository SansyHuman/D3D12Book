#pragma once

#ifndef HLSLTYPE_USE_MATRICES
#define HLSLTYPE_USE_MATRICES
#endif

#include "Common/framework.h"
#include "Common/d3dApp.h"
#include "Common/GeometryGenerator.h"
#include "Common/DDSTextureLoader.h"
#include <ppl.h>

#ifndef D3D12BOOK_VECADD_H
#define D3D12BOOK_VECADD_H

using namespace DirectX;
using namespace DirectX::PackedVector;

constexpr int MaxLights = 16;

struct Vertex
{
    float3 Pos;
    float3 Normal;
    float2 TexC;
};

struct TreeSpriteVertex
{
    float3 Pos;
    float2 Size;
};

struct Data
{
    float3 v1;
    float2 v2;
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

    std::unique_ptr<DirectXHelper::UploadBuffer<Vertex>> WavesVB = nullptr;

    UINT64 Fence = 0;

    FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount, UINT waveVertCount)
    {
        ThrowIfFailed(device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(CmdListAlloc.GetAddressOf())
        ));

        PassCB = std::make_unique<DirectXHelper::UploadBuffer<PassConstants>>(device, passCount, true);
        ObjectCB = std::make_unique<DirectXHelper::UploadBuffer<ObjectConstants>>(device, objectCount, true);
        MaterialCB = std::make_unique<DirectXHelper::UploadBuffer<DirectXHelper::MaterialConstants>>(device, materialCount, true);

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
    std::vector<float2> mTexC;
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
    const float2& TexC(int i) const { return mTexC[i]; }

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
    mTexC.resize(m * n);

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
            mTexC[i * n + j] = float2((float)j / (n - 1), (float)i / (m - 1));
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
    AlphaTested,
    AlphaTestedTreeSprites,
    Transparent,
    Count
};

class VecAdd : public D3DApp
{
private:
    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrFrameResource = nullptr;
    int mCurrFrameResourceIndex = 0;

    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    ComPtr<ID3D12RootSignature> mCSRootSignature = nullptr;

    ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap;
    ComPtr<ID3D12DescriptorHeap> mSamplerDescriptorHeap;

    ComPtr<ID2D1SolidColorBrush> mColorBrush;
    ComPtr<IDWriteTextFormat> mUITextFormat;

    ComPtr<ID3D12Resource> mInputBufferA;
    ComPtr<ID3D12Resource> mInputUploadBufferA;
    ComPtr<ID3D12Resource> mInputBufferB;
    ComPtr<ID3D12Resource> mInputUploadBufferB;
    ComPtr<ID3D12Resource> mOutputBuffer;
    ComPtr<ID3D12Resource> mReadBackBuffer;

    std::unordered_map<std::wstring, std::unique_ptr<DirectXHelper::MeshGeometry>> mGeometries;
    std::unordered_map<std::wstring, std::unique_ptr<DirectXHelper::Material>> mMaterials;
    std::unordered_map<std::wstring, std::unique_ptr<DirectXHelper::Texture>> mTextures;
    std::unordered_map<std::wstring, ComPtr<ID3DBlob>> mShaders;
    std::unordered_map<std::wstring, ComPtr<ID3D12PipelineState>> mPSOs;

    int mMaxSrvHeapSize = 0;
    int mMaxMaterialNumber = 0;
    int mMaxRenderItemNumber = 0;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    std::vector<D3D12_INPUT_ELEMENT_DESC> mTreeSpriteInputLayout;

    RenderItem* mWavesRItem = nullptr;

    std::vector<std::unique_ptr<RenderItem>> mAllRItems;
    std::vector<RenderItem*> mRItemLayer[(int)RenderLayer::Count];

    std::unique_ptr<Waves> mWaves;

    PassConstants mMainPassCB;

    float3 mEyePos = { 0.0f, 0.0f, 0.0f };
    float4x4 mView = DirectXHelper::Math::Identity4X4();
    float4x4 mProj = DirectXHelper::Math::Identity4X4();

    float mPhi = 1.5f * XM_PI;
    float mTheta = XM_PIDIV4;
    float mRadius = 30.0f;

    float mSunPhi = 1.25f * XM_PI;
    float mSunTheta = XM_PIDIV4;

    float mGamma = 2.2f;

    POINT mLastMousePos;

public:
    VecAdd(HINSTANCE hInstance);
    VecAdd(const VecAdd&) = delete;
    VecAdd& operator=(const VecAdd&) = delete;
    ~VecAdd();

    virtual bool Initialize(const D3DApp::D3DAPP_SETTINGS& settings) override;

    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

private:
    virtual void OnResize() override;
    virtual void Update(GameTimer<float>& gt) override;
    virtual void Draw(const GameTimer<float>& gt) override;
    virtual void RenderUI(const GameTimer<float>& gt) override;

    void DoComputeWork();

    virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

    void OnKeyboardInput(const GameTimer<float>& gt);
    void UpdateCamera(const GameTimer<float>& gt);
    void AnimateMaterials(const GameTimer<float>& gt);
    void UpdateObjectCBs(const GameTimer<float>& gt);
    void UpdateMaterialCBs(const GameTimer<float>& gt);
    void UpdateMainPassCB(const GameTimer<float>& gt);
    void UpdateWaves(const GameTimer<float>& gt);

    void BuildBuffers();
    void LoadTextures();
    void BuildRootSignature();
    void BuildDescriptorHeaps();
    void BuildShadersAndInputLayout();
    void BuildGeometry();
    void BuildWavesGeometryBuffers();
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

    float GetHillsHeight(float x, float z) const;
    float3 GetHillsNormal(float x, float z) const;
};

VecAdd::VecAdd(HINSTANCE hInstance)
    : D3DApp(hInstance)
{
}

VecAdd::~VecAdd()
{
    if(md3dDevice != nullptr)
        FlushCommandQueue();
}

bool VecAdd::Initialize(const D3DApp::D3DAPP_SETTINGS& settings)
{
    if(!D3DApp::Initialize(settings))
        return false;

    ThrowIfFailed(md2dDeviceContext->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::Black),
        mColorBrush.GetAddressOf()
    ));

    ThrowIfFailed(mdwriteFactory->CreateTextFormat(
        L"Times New Roman",
        nullptr,
        DWRITE_FONT_WEIGHT_BLACK,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        40.0f,
        L"",
        mUITextFormat.GetAddressOf()
    ));

    ThrowIfFailed(mUITextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING));
    ThrowIfFailed(mUITextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER));

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    mWaves = std::make_unique<Waves>(320, 320, 0.5f, 0.016f, 5.0f, 0.4f);

    BuildBuffers();
    LoadTextures();
    BuildRootSignature();
    BuildDescriptorHeaps();
    BuildShadersAndInputLayout();
    BuildGeometry();
    BuildWavesGeometryBuffers();
    BuildMaterials();
    BuildRenderItems();
    BuildFrameResources();
    BuildPSOs();

    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdList[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(1, cmdList);

    FlushCommandQueue();

    DoComputeWork();

    return true;
}

LRESULT VecAdd::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

void VecAdd::OnResize()
{
    D3DApp::OnResize();

    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * XM_PI, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);
}

void VecAdd::Update(GameTimer<float>& gt)
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
    UpdateWaves(gt);
}

void VecAdd::Draw(const GameTimer<float>& gt)
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
    mCommandList->SetGraphicsRootConstantBufferView(4, mCurrFrameResource->PassCB->Resource()->GetGPUVirtualAddress());

    mCommandList->SetPipelineState(mPSOs[L"opaque"].Get());
    DrawRenderItems(mCommandList.Get(), mRItemLayer[(int)RenderLayer::Opaque]);

    mCommandList->SetPipelineState(mPSOs[L"alphaTested"].Get());
    DrawRenderItems(mCommandList.Get(), mRItemLayer[(int)RenderLayer::AlphaTested]);

    mCommandList->SetPipelineState(mPSOs[L"treeSprites"].Get());
    DrawRenderItems(mCommandList.Get(), mRItemLayer[(int)RenderLayer::AlphaTestedTreeSprites]);

    mCommandList->SetPipelineState(mPSOs[L"transparent"].Get());
    DrawRenderItems(mCommandList.Get(), mRItemLayer[(int)RenderLayer::Transparent]);

    D3D12_RESOURCE_BARRIER rtPresent = CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );
    mCommandList->ResourceBarrier(1, &rtPresent);

    ThrowIfFailed(mCommandList->Close());

    ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    RenderUI(gt);

    ThrowIfFailed(mSwapChain->Present(0, 0));
    mCurrentBackBuffer = (mCurrentBackBuffer + 1) % mSwapChainBufferCount;

    mCurrFrameResource->Fence = ++mCurrentFence;

    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void VecAdd::RenderUI(const GameTimer<float>& gt)
{
    D2D1_SIZE_F rtSize = md2dRenderTargets[mCurrentBackBuffer]->GetSize();

    md3d11On12Device->AcquireWrappedResources(mWrappedBackBuffers[mCurrentBackBuffer].GetAddressOf(), 1);

    md2dDeviceContext->SetTarget(md2dRenderTargets[mCurrentBackBuffer].Get());
    md2dDeviceContext->SetUnitMode(D2D1_UNIT_MODE_DIPS);
    float dpiX, dpiY;
    md2dRenderTargets[mCurrentBackBuffer]->GetDpi(&dpiX, &dpiY);
    md2dDeviceContext->SetDpi(dpiX, dpiY);

    md2dDeviceContext->BeginDraw();

    md2dDeviceContext->SetTransform(D2D1::IdentityMatrix());

    mColorBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
    md2dDeviceContext->DrawTextW(
        mFpsText.c_str(),
        (UINT32)mFpsText.size(),
        mUITextFormat.Get(),
        { 25.0f, 15.0f, rtSize.width - 15.0f, 65.0f },
        mColorBrush.Get()
    );

    mColorBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
    md2dDeviceContext->DrawTextW(
        mFpsText.c_str(),
        (UINT32)mFpsText.size(),
        mUITextFormat.Get(),
        { 20.0f, 10.0f, rtSize.width - 20.0f, 60.0f },
        mColorBrush.Get()
    );

    ThrowIfFailed(md2dDeviceContext->EndDraw());

    md3d11On12Device->ReleaseWrappedResources(mWrappedBackBuffers[mCurrentBackBuffer].GetAddressOf(), 1);

    md3d11DeviceContext->Flush();
}

void VecAdd::DoComputeWork()
{
    ThrowIfFailed(mDirectCmdListAlloc->Reset());

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSOs[L"vecAdd"].Get()));

    mCommandList->SetComputeRootSignature(mCSRootSignature.Get());

    mCommandList->SetComputeRootShaderResourceView(0, mInputBufferA->GetGPUVirtualAddress());
    mCommandList->SetComputeRootShaderResourceView(1, mInputBufferB->GetGPUVirtualAddress());
    mCommandList->SetComputeRootUnorderedAccessView(2, mOutputBuffer->GetGPUVirtualAddress());

    mCommandList->Dispatch(1, 1, 1);

    D3D12_RESOURCE_BARRIER outputCopy = CD3DX12_RESOURCE_BARRIER::Transition(
        mOutputBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_COPY_SOURCE
    );
    mCommandList->ResourceBarrier(1, &outputCopy);

    mCommandList->CopyResource(mReadBackBuffer.Get(), mOutputBuffer.Get());

    D3D12_RESOURCE_BARRIER outputCommon = CD3DX12_RESOURCE_BARRIER::Transition(
        mOutputBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_SOURCE,
        D3D12_RESOURCE_STATE_COMMON
    );
    mCommandList->ResourceBarrier(1, &outputCommon);

    ThrowIfFailed(mCommandList->Close());

    ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    FlushCommandQueue();

    Data* mappedData = nullptr;
    ThrowIfFailed(mReadBackBuffer->Map(0, nullptr, (void**)&mappedData));

    std::ofstream fout("results.txt");

    for(int i = 0; i < 32; i++)
    {
        fout << "(" << mappedData[i].v1.x << ", " << mappedData[i].v1.y << ", " << mappedData[i].v1.z <<
            ", " << mappedData[i].v2.x << ", " << mappedData[i].v2.y << ")" << std::endl;
    }

    fout.close();

    mReadBackBuffer->Unmap(0, nullptr);
}

void VecAdd::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;
    SetCapture(mhMainWnd);
}

void VecAdd::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void VecAdd::OnMouseMove(WPARAM btnState, int x, int y)
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

void VecAdd::OnKeyboardInput(const GameTimer<float>& gt)
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
}

void VecAdd::UpdateCamera(const GameTimer<float>& gt)
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

void VecAdd::AnimateMaterials(const GameTimer<float>& gt)
{
    auto waterMat = mMaterials[L"water"].get();

    float& tu = waterMat->MatTransform(3, 0);
    float& tv = waterMat->MatTransform(3, 1);

    tu += 0.1f * gt.DeltaTime();
    tv += 0.02f * gt.DeltaTime();

    if(tu >= 1.0f)
    {
        tu -= 1.0f;
    }
    if(tv >= 1.0f)
    {
        tv -= 1.0f;
    }

    waterMat->NumFramesDirty = gNumFrameResources;
}

void VecAdd::UpdateObjectCBs(const GameTimer<float>& gt)
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

void VecAdd::UpdateMaterialCBs(const GameTimer<float>& gt)
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

void VecAdd::UpdateMainPassCB(const GameTimer<float>& gt)
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

    mMainPassCB.FogStart = 100.0f;
    mMainPassCB.FogRange = 300.0f;
    mMainPassCB.FogColor = { 0.7f, 0.7f, 0.7f, 1.0f };

    mMainPassCB.AmbientLight = { 0.1f, 0.1f, 0.1f, 1.0f };

    mMainPassCB.Lights[0].Type = (int)DirectXHelper::LIGHT_TYPE_DIRECTIONAL;
    XMVECTOR lightDir = -DirectXHelper::Math::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);
    XMStoreFloat3(&mMainPassCB.Lights[0].Direction, lightDir);
    mMainPassCB.Lights[0].Strength = { 0.9f, 0.9f, 0.9f };

    mMainPassCB.Lights[1].Type = (int)DirectXHelper::LIGHT_TYPE_POINT;
    mMainPassCB.Lights[1].Strength = { 0.1f, 0.9f, 0.1f };
    mMainPassCB.Lights[1].FalloffStart = 3.0f;
    mMainPassCB.Lights[1].FalloffEnd = 16.0f;
    mMainPassCB.Lights[1].Position = { 0.2f, 8.0f, 0.0f };

    mMainPassCB.Lights[2].Type = (int)DirectXHelper::LIGHT_TYPE_SPOTLIGHT;
    mMainPassCB.Lights[2].Strength = { 1.0f, 0.0f, 0.2f };
    mMainPassCB.Lights[2].FalloffStart = 30.0f;
    mMainPassCB.Lights[2].FalloffEnd = 200.0f;
    lightDir = XMVectorSet(1.0f, -8.0f, 1.5f, 1.0f);
    lightDir = XMVector3Normalize(lightDir);
    XMStoreFloat3(&mMainPassCB.Lights[2].Direction, lightDir);
    mMainPassCB.Lights[2].Position = { -50.0f, 40.0f, 0.0f };
    mMainPassCB.Lights[2].SpotPower = 10.0f;

    mMainPassCB.NumLights = 3;

    DirectXHelper::UploadBuffer<PassConstants>* currPassCB = mCurrFrameResource->PassCB.get();
    currPassCB->CopyData(0, mMainPassCB);
}

void VecAdd::UpdateWaves(const GameTimer<float>& gt)
{
    static float t_base = 0.0f;

    if((gt.TotalTime() - t_base) >= 0.1f)
    {
        t_base = gt.TotalTime();

        int i = DirectXHelper::Math::Rand(4, mWaves->RowCount() - 5);
        int j = DirectXHelper::Math::Rand(4, mWaves->ColumnCount() - 5);

        float r = DirectXHelper::Math::RandF(0.7f, 1.4f) * ((float)DirectXHelper::Math::Rand(0, 2) * 1.5f - 1.0f);

        mWaves->Disturb(i, j, r);
    }

    mWaves->Update(gt.DeltaTime());

    DirectXHelper::UploadBuffer<Vertex>* currWavesVB = mCurrFrameResource->WavesVB.get();
    for(int i = 0; i < mWaves->VertexCount(); i++)
    {
        Vertex v = {};

        v.Pos = mWaves->Position(i);
        v.Normal = mWaves->Normal(i);
        v.TexC = mWaves->TexC(i);

        currWavesVB->CopyData(i, v);
    }

    mWavesRItem->Geo->VertexBufferGPU = currWavesVB->Resource();
}

void VecAdd::BuildBuffers()
{
    std::array<Data, 32> dataA;
    std::array<Data, 32> dataB;
    for(size_t i = 0; i < dataA.size(); i++)
    {
        dataA[i].v1 = float3(i, i, i);
        dataA[i].v2 = float2(i, 0);

        dataB[i].v1 = float3(-(int)i, i, 0.0f);
        dataB[i].v2 = float2(0, -(int)i);
    }

    UINT64 byteSize = dataA.size() * sizeof(Data);

    ThrowIfFailed(DirectXHelper::CreateDefaultBuffer(
        md3dDevice.Get(),
        mCommandList.Get(),
        dataA.data(),
        byteSize,
        mInputBufferA.GetAddressOf(),
        mInputUploadBufferA.GetAddressOf()
    ));
    ThrowIfFailed(DirectXHelper::CreateDefaultBuffer(
        md3dDevice.Get(),
        mCommandList.Get(),
        dataB.data(),
        byteSize,
        mInputBufferB.GetAddressOf(),
        mInputUploadBufferB.GetAddressOf()
    ));

    CD3DX12_HEAP_PROPERTIES outputHeap(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC outputBuffer = CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &outputHeap,
        D3D12_HEAP_FLAG_NONE,
        &outputBuffer,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        IID_PPV_ARGS(mOutputBuffer.GetAddressOf())
    ));

    CD3DX12_HEAP_PROPERTIES readBackHeap(D3D12_HEAP_TYPE_READBACK);
    CD3DX12_RESOURCE_DESC readBackBuffer = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &readBackHeap,
        D3D12_HEAP_FLAG_NONE,
        &readBackBuffer,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(mReadBackBuffer.GetAddressOf())
    ));
}

void VecAdd::LoadTextures()
{
    DDS_TEXTURE_FILE_INFO textureFiles[] = {
        L"grassTex", L"Textures/grass.dds", D3D12_SRV_DIMENSION_TEXTURE2D,
        L"waterTex", L"Textures/water1.dds", D3D12_SRV_DIMENSION_TEXTURE2D,
        L"woodCrateTex", L"Textures/WireFence.dds", D3D12_SRV_DIMENSION_TEXTURE2D,
        L"treeArrayTex", L"Textures/treeArray2.dds", D3D12_SRV_DIMENSION_TEXTURE2DARRAY
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

void VecAdd::BuildRootSignature()
{
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

    {
        CD3DX12_ROOT_PARAMETER slotRootParameter[3];

        slotRootParameter[0].InitAsShaderResourceView(0);
        slotRootParameter[1].InitAsShaderResourceView(1);
        slotRootParameter[2].InitAsUnorderedAccessView(0);

        CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

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
            IID_PPV_ARGS(mCSRootSignature.GetAddressOf())));
    }
}

static void BuildSrvDesc(D3D12_SHADER_RESOURCE_VIEW_DESC& desc, DirectXHelper::Texture* tex)
{
    desc.Format = tex->Resource->GetDesc().Format;
    desc.ViewDimension = tex->Dimension;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    switch(desc.ViewDimension)
    {
    case D3D12_SRV_DIMENSION_TEXTURE1D:
        desc.Texture1D.MostDetailedMip = 0;
        desc.Texture1D.MipLevels = tex->Resource->GetDesc().MipLevels;
        desc.Texture1D.ResourceMinLODClamp = 0.0f;
        break;
    case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
        desc.Texture1DArray.MostDetailedMip = 0;
        desc.Texture1DArray.MipLevels = tex->Resource->GetDesc().MipLevels;
        desc.Texture1DArray.FirstArraySlice = 0;
        desc.Texture1DArray.ArraySize = tex->Resource->GetDesc().DepthOrArraySize;
        desc.Texture1DArray.ResourceMinLODClamp = 0.0f;
        break;
    case D3D12_SRV_DIMENSION_TEXTURE2D:
        desc.Texture2D.MostDetailedMip = 0;
        desc.Texture2D.MipLevels = tex->Resource->GetDesc().MipLevels;
        desc.Texture2D.PlaneSlice = 0;
        desc.Texture2D.ResourceMinLODClamp = 0.0f;
        break;
    case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
        desc.Texture2DArray.MostDetailedMip = 0;
        desc.Texture2DArray.MipLevels = tex->Resource->GetDesc().MipLevels;
        desc.Texture2DArray.FirstArraySlice = 0;
        desc.Texture2DArray.ArraySize = tex->Resource->GetDesc().DepthOrArraySize;
        desc.Texture2DArray.PlaneSlice = 0;
        desc.Texture2DArray.ResourceMinLODClamp = 0.0f;
        break;
    case D3D12_SRV_DIMENSION_TEXTURE3D:
        desc.Texture3D.MostDetailedMip = 0;
        desc.Texture3D.MipLevels = tex->Resource->GetDesc().MipLevels;
        desc.Texture3D.ResourceMinLODClamp = 0.0f;
        break;
    }
}

void VecAdd::BuildDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = mMaxSrvHeapSize;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(mSrvDescriptorHeap.GetAddressOf())));

    CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

    for(auto& tex : mTextures)
    {
        BuildSrvDesc(srvDesc, tex.second.get());

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

void VecAdd::BuildShadersAndInputLayout()
{
    const D3D_SHADER_MACRO alphaTestDefines[] = {
        "ALPHA_TEST", "1",
        "FOG", "1",
        nullptr, nullptr
    };

    const D3D_SHADER_MACRO opaqueDefines[] = {
        "FOG", "1",
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

    mShaders[L"treeSpriteVS"] = DirectXHelper::CompileShader(
        L"Shaders\\TreeSprite.hlsl",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "VS",
        "vs_5_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0
    );

    mShaders[L"treeSpriteGS"] = DirectXHelper::CompileShader(
        L"Shaders\\TreeSprite.hlsl",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "GS",
        "gs_5_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0
    );

    mShaders[L"treeSpritePS"] = DirectXHelper::CompileShader(
        L"Shaders\\TreeSprite.hlsl",
        alphaTestDefines,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "PS",
        "ps_5_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0
    );

    mShaders[L"vecAddCS"] = DirectXHelper::CompileShader(
        L"Shaders\\VecAdd.hlsl",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "CS",
        "cs_5_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0
    );

    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    mTreeSpriteInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
}

void VecAdd::BuildGeometry()
{
    {
        GeometryGenerator::MeshData<std::uint32_t> grid = GeometryGenerator::CreateGrid<std::uint32_t>(160.0f, 160.0f, 160, 160);

        std::vector<Vertex> vertices(grid.Vertices.size());
        for(size_t i = 0; i < grid.Vertices.size(); i++)
        {
            float3& p = grid.Vertices[i].Position;

            vertices[i].Pos = p;
            vertices[i].Pos.y = GetHillsHeight(p.x, p.z);
            vertices[i].Normal = GetHillsNormal(p.x, p.z);
            vertices[i].TexC = grid.Vertices[i].TexC;
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

    {
        GeometryGenerator::MeshData<std::uint16_t> box = GeometryGenerator::CreateBox<std::uint16_t>(1.0f, 1.0f, 1.0f, 3);

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

    {
        static constexpr std::size_t treeCount = 32;
        std::array<TreeSpriteVertex, treeCount> vertices;
        for(std::size_t i = 0; i < treeCount; i++)
        {
            float x = DirectXHelper::Math::RandF(-70.0f, 70.0f);
            float z = DirectXHelper::Math::RandF(-70.0f, 70.0f);
            float y = GetHillsHeight(x, z);
            if(y < 0.0f)
            {
                i--;
                continue;
            }
            y += 8.0f;

            vertices[i].Pos = float3(x, y, z);
            vertices[i].Size = float2(20.0f, 20.0f);
        }

        std::array<std::uint16_t, treeCount> indices;
        for(std::size_t i = 0; i < treeCount; i++)
        {
            indices[i] = i;
        }

        const UINT vbByteSize = (UINT)vertices.size() * sizeof(TreeSpriteVertex);
        const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

        auto geo = std::make_unique<DirectXHelper::MeshGeometry>();

        geo->Name = L"treeSpritesGeo";

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

        geo->VertexByteStride = sizeof(TreeSpriteVertex);
        geo->VertexBufferByteSize = vbByteSize;
        geo->IndexFormat = DXGI_FORMAT_R16_UINT;
        geo->IndexBufferByteSize = ibByteSize;

        DirectXHelper::MeshGeometry::SubmeshGeometry submesh = {};
        submesh.IndexCount = (UINT)indices.size();
        submesh.StartIndexLocation = 0;
        submesh.BaseVertexLocation = 0;

        geo->DrawArgs[L"points"] = submesh;

        mGeometries[geo->Name] = std::move(geo);
    }
}

void VecAdd::BuildWavesGeometryBuffers()
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

void VecAdd::BuildPSOs()
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
    transparentPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(mPSOs[L"transparent"].GetAddressOf())));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC treeSpritePsoDesc = opaquePsoDesc;
    treeSpritePsoDesc.InputLayout.pInputElementDescs = mTreeSpriteInputLayout.data();
    treeSpritePsoDesc.InputLayout.NumElements = mTreeSpriteInputLayout.size();
    treeSpritePsoDesc.VS.pShaderBytecode = (BYTE*)mShaders[L"treeSpriteVS"]->GetBufferPointer();
    treeSpritePsoDesc.VS.BytecodeLength = mShaders[L"treeSpriteVS"]->GetBufferSize();
    treeSpritePsoDesc.GS.pShaderBytecode = (BYTE*)mShaders[L"treeSpriteGS"]->GetBufferPointer();
    treeSpritePsoDesc.GS.BytecodeLength = mShaders[L"treeSpriteGS"]->GetBufferSize();
    treeSpritePsoDesc.PS.pShaderBytecode = (BYTE*)mShaders[L"treeSpritePS"]->GetBufferPointer();
    treeSpritePsoDesc.PS.BytecodeLength = mShaders[L"treeSpritePS"]->GetBufferSize();
    treeSpritePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    treeSpritePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&treeSpritePsoDesc, IID_PPV_ARGS(mPSOs[L"treeSprites"].GetAddressOf())));

    D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
    computePsoDesc.pRootSignature = mCSRootSignature.Get();
    computePsoDesc.CS.pShaderBytecode = mShaders[L"vecAddCS"]->GetBufferPointer();
    computePsoDesc.CS.BytecodeLength = mShaders[L"vecAddCS"]->GetBufferSize();
    computePsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    ThrowIfFailed(md3dDevice->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(mPSOs[L"vecAdd"].GetAddressOf())));
}

void VecAdd::BuildFrameResources()
{
    for(int i = 0; i < gNumFrameResources; i++)
    {
        mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(), 1, (UINT)mAllRItems.size(), (UINT)mMaterials.size(), mWaves->VertexCount()));
    }
}

void VecAdd::BuildMaterials()
{
    DirectXHelper::MaterialConstants matConst;

    matConst.DiffuseAlbedo = float4(1.0f, 1.0f, 1.0f, 1.0f);
    matConst.FresnelR0 = float3(0.01f, 0.01f, 0.01f);
    matConst.Roughness = 0.125f;
    AddMaterial(L"grass", L"grassTex", matConst);

    matConst.DiffuseAlbedo = float4(1.0f, 1.0f, 1.0f, 0.5f);
    matConst.FresnelR0 = float3(0.1f, 0.1f, 0.1f);
    matConst.Roughness = 0.0f;
    AddMaterial(L"water", L"waterTex", matConst);

    matConst.DiffuseAlbedo = float4(1.0f, 1.0f, 1.0f, 1.0f);
    matConst.FresnelR0 = float3(0.05f, 0.05f, 0.05f);
    matConst.Roughness = 0.2f;
    AddMaterial(L"woodCrate", L"woodCrateTex", matConst);

    matConst.DiffuseAlbedo = float4(1.0f, 1.0f, 1.0f, 1.0f);
    matConst.FresnelR0 = float3(0.01f, 0.01f, 0.01f);
    matConst.Roughness = 0.8f;
    AddMaterial(L"treeSprites", L"treeArrayTex", matConst);
}

void VecAdd::BuildRenderItems()
{
    float4x4 world = DirectXHelper::Math::Identity4X4();
    float4x4 texTransform = DirectXHelper::Math::Identity4X4();

    XMStoreFloat4x4(&texTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
    mWavesRItem = AddRenderItem(
        RenderLayer::Transparent,
        L"waterGeo",
        L"grid",
        L"water",
        world,
        texTransform
    );

    XMStoreFloat4x4(&texTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
    AddRenderItem(
        RenderLayer::Opaque,
        L"landGeo",
        L"grid",
        L"grass",
        world,
        texTransform
    );

    XMStoreFloat4x4(&world, XMMatrixScaling(10.0f, 10.0f, 10.0f) * XMMatrixTranslation(-10.0f, 3.0f, 10.0f));
    texTransform = DirectXHelper::Math::Identity4X4();
    AddRenderItem(
        RenderLayer::AlphaTested,
        L"boxGeo",
        L"box",
        L"woodCrate",
        world,
        texTransform
    );

    world = DirectXHelper::Math::Identity4X4();
    texTransform = DirectXHelper::Math::Identity4X4();
    AddRenderItem(
        RenderLayer::AlphaTestedTreeSprites,
        L"treeSpritesGeo",
        L"points",
        L"treeSprites",
        world,
        texTransform,
        D3D11_PRIMITIVE_TOPOLOGY_POINTLIST
    );
}

void VecAdd::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& rItems)
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

void VecAdd::AddTexture(DDS_TEXTURE& texture)
{
    auto texData = std::make_unique<DirectXHelper::Texture>();
    texData->Name = texture.TextureFile.TextureName;
    texData->Filename = texture.TextureFile.TextureFileUrl;
    texData->Dimension = texture.TextureFile.Dimension;
    texData->SrvHeapIndex = mMaxSrvHeapSize;
    texData->Resource = texture.Texture;
    texData->UploadHeap = texture.TextureUploadHeap;

    mMaxSrvHeapSize++;

    mTextures[texData->Name] = std::move(texData);
}

void VecAdd::AddMaterial(LPCWSTR name, LPCWSTR diffuseTexture, DirectXHelper::MaterialConstants& matConst)
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

RenderItem* VecAdd::AddRenderItem(RenderLayer layer, LPCWSTR geometry, LPCWSTR subgeometry, LPCWSTR material, float4x4& worldTransform, float4x4& textureTransform, D3D12_PRIMITIVE_TOPOLOGY primitiveType)
{
    std::unique_ptr<RenderItem> renderItem = std::make_unique<RenderItem>();
    renderItem->World = worldTransform;
    renderItem->TexTransform = textureTransform;
    renderItem->ObjCBIndex = mMaxRenderItemNumber;
    renderItem->Mat = mMaterials[material].get();
    renderItem->Geo = mGeometries[geometry].get();
    renderItem->PrimitiveType = primitiveType;
    renderItem->IndexCount = renderItem->Geo->DrawArgs[subgeometry].IndexCount;
    renderItem->StartIndexLocation = renderItem->Geo->DrawArgs[subgeometry].StartIndexLocation;
    renderItem->BaseVertexLocation = renderItem->Geo->DrawArgs[subgeometry].BaseVertexLocation;

    mMaxRenderItemNumber++;

    mRItemLayer[(int)layer].push_back(renderItem.get());
    RenderItem* ret = renderItem.get();
    mAllRItems.push_back(std::move(renderItem));

    return ret;
}

float VecAdd::GetHillsHeight(float x, float z) const
{
    return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

float3 VecAdd::GetHillsNormal(float x, float z) const
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