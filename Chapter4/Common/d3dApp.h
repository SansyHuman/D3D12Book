#pragma once

#include "d3dUtil.h"
#include "GameTimer.h"

#ifndef D3D12BOOK_D3DAPP_H
#define D3D12BOOK_D3DAPP_H

using namespace Microsoft::WRL;
using namespace DirectX;

class D3DApp
{
protected:
    static D3DApp* mApp;

    HINSTANCE mhAppInst = nullptr;
    HWND mhMainWnd = nullptr;
    bool mAppPaused = false;
    bool mMinimized = false;
    bool mMaximized = false;
    bool mResizing = false;
    bool mFullscreenState = false;

    GameTimer<float> mTimer;

    ComPtr<IDXGIFactory4> mdxgiFactory;
    ComPtr<IDXGISwapChain> mSwapChain;

    D3D_FEATURE_LEVEL mFeatureLevel = D3D_FEATURE_LEVEL_11_0;
    ComPtr<ID3D12Device> md3dDevice;

    ComPtr<ID3D12Fence> mFence;
    UINT64 mCurrentFence = 0;

    UINT mRtvDescriptorSize = 0;
    UINT mDsvDescriptorSize = 0;
    UINT mCbvSrvDescriptorSize = 0;

    UINT mMsaaSampleCount = 4;
    UINT mMsaaQuality = 0;
    bool mMsaaState = false;

    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;

    ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    ComPtr<ID3D12DescriptorHeap> mDsvHeap;

    int mCurrentBackBuffer = 0;
    std::vector<ComPtr<ID3D12Resource>> mSwapChainBuffers;
    ComPtr<ID3D12Resource> mDepthStencilBuffer;

    D3D12_VIEWPORT mScreenViewport = {};
    D3D12_RECT mScissorRect = {};

    std::wstring mMainWndCaption = L"d3d App";
    D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    UINT mSwapChainBufferCount = 2;
    UINT mClientWidth = 800;
    UINT mClientHeight = 600;
    DXGI_RATIONAL mRefreshRate = { 60, 1 };

protected:
    D3DApp(HINSTANCE hInstance);
    D3DApp(const D3DApp&) = delete;
    D3DApp& operator=(const D3DApp&) = delete;
    virtual ~D3DApp();

public:
    static D3DApp* GetApp();

    HINSTANCE AppInst() const;
    HWND MainWnd() const;
    float AspectRatio() const;

    bool GetMsaaState() const;
    void SetMsaaState(bool active, UINT sampleCount = 0);

    int Run();

    struct D3DAPP_SETTINGS
    {
    public:
        D3D_FEATURE_LEVEL FeatureLevel;
        UINT MsaaSampleCount;
        LPCWSTR MainWindowCaption;
        UINT ClientWidth;
        UINT ClientHeight;
        DXGI_RATIONAL RefreshRate;
        UINT SwapChainBufferCount;
        D3D_DRIVER_TYPE DriverType;
        DXGI_FORMAT BackBufferFormat;
        DXGI_FORMAT DepthStencilFormat;

        static D3DAPP_SETTINGS Default(D3D_FEATURE_LEVEL featureLevel, UINT msaaSampleCount)
        {
            D3DAPP_SETTINGS settings = {};
            settings.FeatureLevel = featureLevel;
            settings.MsaaSampleCount = msaaSampleCount;
            settings.MainWindowCaption = L"d3d App";
            settings.ClientWidth = 800;
            settings.ClientHeight = 600;
            settings.RefreshRate.Numerator = 60;
            settings.RefreshRate.Denominator = 1;
            settings.SwapChainBufferCount = 2;
            settings.DriverType = D3D_DRIVER_TYPE_HARDWARE;
            settings.BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
            settings.DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

            return settings;
        }
    };

    virtual bool Initialize(const D3DApp::D3DAPP_SETTINGS& settings);

    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
    bool InitMainWindow();
    bool InitDirect3D();

    void EnableDebugLayer();
    void CreateD3DDevice();
    void CreateFence();
    void GetDescriptorSizes();
    void CheckMsaaSupport();
    void CreateCommandObjects();
    void CreateSwapChain();
    virtual void CreateRtvAndDsvDescriptorHeaps();

    void CreateRtv();
    void CreateDsv();
    void FlushCommandQueue();

    virtual void OnResize();

    virtual void Start() {}

    virtual void Update(GameTimer<float>& gt) {}
    virtual void LateUpdate(GameTimer<float>& gt) {}

    virtual void Draw(const GameTimer<float>& gt) {}

    virtual void OnAppPause() {}

    virtual void OnAppQuit() {}

    virtual void OnMouseDown(WPARAM btnState, int x, int y) {}
    virtual void OnMouseUp(WPARAM btnState, int x, int y) {}
    virtual void OnMouseMove(WPARAM btnState, int x, int y) {}

    void PauseApp();
    void UnpauseApp();
    void QuitApp(int exitCode);

    ID3D12Resource* CurrentBackBuffer() const;
    D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
    D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

    void CalculateFrameStats();

    void LogAdapters();
    void LogAdapterOutputs(IDXGIAdapter* adapter);
    void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);
};

#endif //D3D12BOOK_D3DAPP_H
