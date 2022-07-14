//
// Created by suboo on 2021-08-11.
//

#include "d3dApp.h"

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return D3DApp::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

D3DApp* D3DApp::mApp = nullptr;

D3DApp::D3DApp(HINSTANCE hInstance)
    : mhAppInst(hInstance)
{
    assert(mApp == nullptr);
    mApp = this;
}

D3DApp::~D3DApp()
{
    if(md3dDevice != nullptr)
        FlushCommandQueue();
}

D3DApp* D3DApp::GetApp()
{
    return D3DApp::mApp;
}

HINSTANCE D3DApp::AppInst() const
{
    return mhAppInst;
}

HWND D3DApp::MainWnd() const
{
    return mhMainWnd;
}

float D3DApp::AspectRatio() const
{
    return (float)mClientWidth / mClientHeight;
}

bool D3DApp::GetMsaaState() const
{
    return mMsaaState;
}

void D3DApp::SetMsaaState(bool active, UINT sampleCount)
{
    if(sampleCount != 0 && mMsaaSampleCount != sampleCount)
    {
        mMsaaSampleCount = sampleCount;
        mMsaaState = active;

        CreateSwapChain();
        OnResize();

        return;
    }

    if(mMsaaState != active)
    {
        mMsaaState = active;

        CreateSwapChain();
        OnResize();

        return;
    }
}

int D3DApp::Run()
{
    MSG msg = { 0 };
    mTimer.Reset();

    Start();

    while(msg.message != WM_QUIT)
    {
        if(PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        else
        {
            mTimer.Tick();
            if(!mAppPaused)
            {
                CalculateFrameStats();
                Update(mTimer);
                LateUpdate(mTimer);
                Draw(mTimer);
            }
            else
            {
                Sleep(16);
            }
        }
    }

    return (int)msg.wParam;
}

bool D3DApp::Initialize(const D3DApp::D3DAPP_SETTINGS& settings)
{
    mFeatureLevel = settings.FeatureLevel;
    mMsaaSampleCount = settings.MsaaSampleCount;
    mMainWndCaption = settings.MainWindowCaption;
    md3dDriverType = settings.DriverType;
    mBackBufferFormat = settings.BackBufferFormat;
    mDepthStencilFormat = settings.DepthStencilFormat;
    mSwapChainBufferCount = settings.SwapChainBufferCount;
    mClientWidth = settings.ClientWidth;
    mClientHeight = settings.ClientHeight;
    mRefreshRate = settings.RefreshRate;

    if(!InitMainWindow())
        return false;

    if(!InitDirect3D())
        return false;

    OnResize();

    return true;
}

LRESULT D3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
    case WM_ACTIVATE:
        if(LOWORD(wParam) == WA_INACTIVE)
        {
            PauseApp();
            mTimer.Stop();
        }
        else
        {
            UnpauseApp();
            mTimer.Start();
        }
        return 0;
    case WM_SIZE:
        mClientWidth = LOWORD(lParam);
        mClientHeight = HIWORD(lParam);
        if(md3dDevice)
        {
            switch(wParam)
            {
            case SIZE_MINIMIZED:
                PauseApp();
                mMinimized = true;
                mMaximized = false;
                break;
            case SIZE_MAXIMIZED:
                UnpauseApp();
                mMinimized = false;
                mMaximized = true;
                OnResize();
                break;
            case SIZE_RESTORED:
                if(mMinimized)
                {
                    UnpauseApp();
                    mMinimized = false;
                    OnResize();
                }
                else if(mMaximized)
                {
                    UnpauseApp();
                    mMaximized = false;
                    OnResize();
                }
                else if(mResizing)
                {

                }
                else
                {
                    OnResize();
                }
                break;
            }
        }
        return 0;
    case WM_ENTERSIZEMOVE:
        PauseApp();
        mResizing = true;
        mTimer.Stop();
        return 0;
    case WM_EXITSIZEMOVE:
        UnpauseApp();
        mResizing = false;
        mTimer.Start();
        OnResize();
        return 0;
    case WM_DESTROY:
        QuitApp(0);
        return 0;
    case WM_MENUCHAR:
        return MAKELRESULT(0, MNC_CLOSE);
    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* pInfo = (MINMAXINFO*)lParam;
        pInfo->ptMinTrackSize.x = 200;
        pInfo->ptMinTrackSize.y = 200;
        return 0;
    }
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_MOUSEMOVE:
        OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_KEYUP:
        if(wParam == VK_ESCAPE)
        {
            QuitApp(0);
        }
        else if((int)wParam == VK_F2)
            SetMsaaState(!mMsaaState);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool D3DApp::InitMainWindow()
{
    std::wstring className = mMainWndCaption + L"(MainWnd)";

    WNDCLASSW wnd = {};
    wnd.style = CS_HREDRAW | CS_VREDRAW;
    wnd.lpfnWndProc = MainWndProc;
    wnd.cbClsExtra = 0;
    wnd.cbWndExtra = 0;
    wnd.hInstance = mhAppInst;
    wnd.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wnd.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wnd.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wnd.lpszMenuName = nullptr;
    wnd.lpszClassName = className.c_str();

    if(!RegisterClassW(&wnd))
    {
        MessageBoxW(nullptr, L"RegisterClass Failed.", L"Error", 0);
        return false;
    }

    RECT R = { 0, 0, (LONG)mClientWidth, (LONG)mClientHeight };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    int width = R.right - R.left;
    int height = R.bottom - R.top;

    mhMainWnd = CreateWindowExW(
        0,
        wnd.lpszClassName,
        mMainWndCaption.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        width,
        height,
        nullptr,
        nullptr,
        mhAppInst,
        nullptr
    );

    if(!mhMainWnd)
    {
        MessageBoxW(nullptr, L"CreateWindow Failed.", L"Error", 0);
        return false;
    }

    ShowWindow(mhMainWnd, SW_SHOW);
    UpdateWindow(mhMainWnd);

    return true;
}

bool D3DApp::InitDirect3D()
{
#if defined(DEBUG) || defined(_DEBUG)
    EnableDebugLayer();
#endif

    CreateD3DDevice();
    CreateFence();
    GetDescriptorSizes();
    CheckMsaaSupport();

#if defined(DEBUG) || defined(_DEBUG)
    LogAdapters();
#endif

    CreateCommandObjects();
    CreateSwapChain();
    CreateRtvAndDsvDescriptorHeaps();

    CreateD3D11On12Device();
    CreateD2DObjects();

    return true;
}

void D3DApp::EnableDebugLayer()
{
    ComPtr<ID3D12Debug> debugController;
    if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
    }
    else
    {
        OutputDebugStringW(L"Failed to enable debug layer.");
    }
}

void D3DApp::CreateD3DDevice()
{
    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));

    HRESULT hr = D3D12CreateDevice(
        nullptr, // default adapter
        mFeatureLevel,
        IID_PPV_ARGS(&md3dDevice)
    );

    if(FAILED(hr))
    {
        ComPtr<IDXGIAdapter> pWarpAdapter;
        ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));
        ThrowIfFailed(D3D12CreateDevice(
            pWarpAdapter.Get(),
            mFeatureLevel,
            IID_PPV_ARGS(&md3dDevice)
        ));
    }
}

void D3DApp::CreateFence()
{
    mCurrentFence = 0;
    ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
}

void D3DApp::GetDescriptorSizes()
{
    mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void D3DApp::CheckMsaaSupport()
{
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS data = {};
    data.Format = mBackBufferFormat;
    data.SampleCount = mMsaaSampleCount;
    data.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    data.NumQualityLevels = 0;

    ThrowIfFailed(DirectXHelper::CheckFeatureSupport(
        md3dDevice.Get(),
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        data
    ));

    mMsaaQuality = data.NumQualityLevels;
    if(mMsaaQuality == 0)
    {
        OutputDebugStringW(L"MSAA for the given sample count is not supported.");
    }
}

void D3DApp::CreateCommandObjects()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

    ThrowIfFailed(DirectXHelper::CreateCommandAllocatorAndList(
        md3dDevice.Get(),
        0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        nullptr,
        mDirectCmdListAlloc.GetAddressOf(),
        mCommandList.GetAddressOf()));

    mCommandList->Close();
}

void D3DApp::CreateSwapChain()
{
    mSwapChain.Reset();

    DXGI_SWAP_CHAIN_DESC desc = {};
    desc.BufferDesc.Width = mClientWidth;
    desc.BufferDesc.Height = mClientHeight;
    desc.BufferDesc.RefreshRate = mRefreshRate;
    desc.BufferDesc.Format = mBackBufferFormat;
    desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    desc.SampleDesc.Count = mMsaaState ? mMsaaSampleCount : 1;
    desc.SampleDesc.Quality = mMsaaState ? (mMsaaQuality - 1) : 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = mSwapChainBufferCount;
    desc.OutputWindow = mhMainWnd;
    desc.Windowed = true;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ThrowIfFailed(mdxgiFactory->CreateSwapChain(mCommandQueue.Get(), &desc, mSwapChain.GetAddressOf()));
}

void D3DApp::CreateRtvAndDsvDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDesc.NumDescriptors = mSwapChainBufferCount;
    rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvDesc.NodeMask = 0;

    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

    D3D12_DESCRIPTOR_HEAP_DESC dsvDesc = {};
    dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvDesc.NumDescriptors = 1U;
    dsvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvDesc.NodeMask = 0;

    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

void D3DApp::CreateD3D11On12Device()
{
    ComPtr<ID3D11Device> d3d11Device;
    ThrowIfFailed(D3D11On12CreateDevice(
        (IUnknown*)md3dDevice.Get(),
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        nullptr,
        0,
        (IUnknown**)mCommandQueue.GetAddressOf(),
        1,
        0,
        d3d11Device.GetAddressOf(),
        md3d11DeviceContext.GetAddressOf(),
        nullptr
    ));

    ThrowIfFailed(d3d11Device.As(&md3d11On12Device));
}

void D3DApp::CreateD2DObjects()
{
    D2D1_DEVICE_CONTEXT_OPTIONS deviceOptions = D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS;
    ThrowIfFailed(D2D1CreateFactory(
        D2D1_FACTORY_TYPE_MULTI_THREADED,
        IID_PPV_ARGS(md2dFactory.GetAddressOf())
    ));

    ComPtr<IDXGIDevice> dxgiDevice;
    ThrowIfFailed(md3d11On12Device.As(&dxgiDevice));
    ThrowIfFailed(md2dFactory->CreateDevice(dxgiDevice.Get(), md2dDevice.GetAddressOf()));
    ThrowIfFailed(md2dDevice->CreateDeviceContext(deviceOptions, md2dDeviceContext.GetAddressOf()));

    ThrowIfFailed(DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        (IUnknown**)mdwriteFactory.GetAddressOf()
    ));
}

void D3DApp::CreateRtv()
{
    md2dDeviceContext->SetTarget(nullptr);

    for(size_t i = 0; i < mSwapChainBuffers.size(); i++)
    {
        md2dRenderTargets[i].Reset();
    }

    for(size_t i = 0; i < mSwapChainBuffers.size(); i++)
    {
        md3d11On12Device->ReleaseWrappedResources(mWrappedBackBuffers[i].GetAddressOf(), 1);
        mWrappedBackBuffers[i].Reset();
    }

    for(size_t i = 0; i < mSwapChainBuffers.size(); i++)
    {
        mSwapChainBuffers[i].Reset();
    }

    md2dRenderTargets.resize(mSwapChainBufferCount);
    mWrappedBackBuffers.resize(mSwapChainBufferCount);
    mSwapChainBuffers.resize(mSwapChainBufferCount);

    md3d11DeviceContext->ClearState();

    while(true)
    {
        try
        {
            ThrowIfFailed(mSwapChain->ResizeBuffers(
                mSwapChainBufferCount,
                mClientWidth, mClientHeight,
                mBackBufferFormat,
                DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
            ));
            break;
        }
        catch(DirectXHelper::DxException& e)
        {
            if(e.ErrorCode == DXGI_ERROR_INVALID_CALL)
                continue;
            else
                throw e;
        }
    }
    
    mCurrentBackBuffer = 0;

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
    for(UINT i = 0; i < mSwapChainBufferCount; i++)
    {
        ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(mSwapChainBuffers[i].GetAddressOf())));

        md3dDevice->CreateRenderTargetView(mSwapChainBuffers[i].Get(), nullptr, rtvHeapHandle);
        rtvHeapHandle.Offset(1, mRtvDescriptorSize);
    }
}

void D3DApp::CreateDsv()
{
    mDepthStencilBuffer.Reset();

    D3D12_RESOURCE_DESC depthStencilDesc = {};
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = mClientWidth;
    depthStencilDesc.Height = mClientHeight;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = mDepthStencilFormat;
    depthStencilDesc.SampleDesc.Count = mMsaaState ? mMsaaSampleCount : 1;
    depthStencilDesc.SampleDesc.Quality = mMsaaState ? (mMsaaQuality - 1) : 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear = {};
    optClear.Format = mDepthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

    D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &heapProperty,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())
    ));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = mDepthStencilFormat;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.Texture2D.MipSlice = 0U;

    md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, DepthStencilView());
}

void D3DApp::CreateD2DRenderTarget()
{
    float dpiX = (float)mClientWidth * 96.0f / mClientHorizontalDIP;
    float dipY = mClientHorizontalDIP * (float)mClientHeight / (float)mClientWidth;
    float dpiY = (float)mClientHeight * 96.0f / dipY;
    D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
        dpiX,
        dpiY
    );

    for(UINT i = 0; i < mSwapChainBufferCount; i++)
    {
        D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
        ThrowIfFailed(md3d11On12Device->CreateWrappedResource(
            (IUnknown*)mSwapChainBuffers[i].Get(),
            &d3d11Flags,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT,
            IID_PPV_ARGS(mWrappedBackBuffers[i].GetAddressOf())
        ));

        ComPtr<IDXGISurface> surface;
        ThrowIfFailed(mWrappedBackBuffers[i].As(&surface));
        ThrowIfFailed(md2dDeviceContext->CreateBitmapFromDxgiSurface(
            surface.Get(),
            &bitmapProperties,
            md2dRenderTargets[i].GetAddressOf()
        ));
    }
}

void D3DApp::FlushCommandQueue()
{
    mCurrentFence++;

    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence))

    if(mFence->GetCompletedValue() < mCurrentFence)
    {
        HANDLE eventHandle = CreateEventExW(nullptr, nullptr, 0, EVENT_ALL_ACCESS);

        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle))
        WaitForSingleObject(eventHandle, INFINITE);

        CloseHandle(eventHandle);
    }
}

void D3DApp::OnResize()
{
    FlushCommandQueue();
    
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    CreateRtv();
    CreateDsv();
    CreateD2DRenderTarget();

    D3D12_RESOURCE_BARRIER dsvTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        mDepthStencilBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_DEPTH_WRITE
    );
    mCommandList->ResourceBarrier(1, &dsvTransition);

    ThrowIfFailed(mCommandList->Close());

    ID3D12CommandList* cmdList[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdList), cmdList);

    FlushCommandQueue();

    mScreenViewport.TopLeftX = 0;
    mScreenViewport.TopLeftY = 0;
    mScreenViewport.Width = (float)mClientWidth;
    mScreenViewport.Height = (float)mClientHeight;
    mScreenViewport.MinDepth = 0.0f;
    mScreenViewport.MaxDepth = 1.0f;

    mScissorRect = { 0, 0, (long)mClientWidth, (long)mClientHeight };
}

void D3DApp::PauseApp()
{
    if(!mAppPaused)
    {
        mAppPaused = true;
        OnAppPause();
    }
}

void D3DApp::UnpauseApp()
{
    if(mAppPaused)
    {
        mAppPaused = false;
    }
}

void D3DApp::QuitApp(int exitCode)
{
    OnAppQuit();
    PostQuitMessage(exitCode);
}

ID3D12Resource* D3DApp::CurrentBackBuffer() const
{
    return mSwapChainBuffers[mCurrentBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::CurrentBackBufferView() const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
        mCurrentBackBuffer,
        mRtvDescriptorSize
    );
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::DepthStencilView() const
{
    return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void D3DApp::CalculateFrameStats()
{
    static int frameCnt = 0;
    static float elapsedTime = 0.0f;

    static float fps = NAN;
    static float mspf = NAN;

    frameCnt++;

    if((mTimer.UnscaledTotalTime() - elapsedTime) >= 1.0f)
    {
        fps = (float)frameCnt;
        mspf = 1000.0f / fps;

        frameCnt = 0;
        elapsedTime = mTimer.UnscaledTotalTime();
    }

    std::wstring windowText = mMainWndCaption +
        L"  fps: " + std::to_wstring(fps) +
        L"  mspf: " + std::to_wstring(mspf) +
        L"  timescale: " + std::to_wstring(mTimer.GetTimescale());

    std::wstring fpsT = std::to_wstring(roundf(fps * 10.0f) / 10.0f);
    std::wstring mspfT = std::to_wstring(roundf(mspf * 100.0f) / 100.0f);
    fpsT.resize(fpsT.find_first_of(L'.') + 2);
    mspfT.resize(mspfT.find_first_of(L'.') + 3);

    mFpsText = L"fps: " + fpsT +
        L" mspf: " + mspfT;

    SetWindowTextW(mhMainWnd, windowText.c_str());
}

void D3DApp::LogAdapters()
{
    UINT i = 0;
    IDXGIAdapter* adapter = nullptr;
    std::vector<IDXGIAdapter*> adapterList;

    while(mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc = {};
        adapter->GetDesc(&desc);

        std::wstring text = L"***Adapter: ";
        text += desc.Description;
        text += L"\n";

        OutputDebugStringW(text.c_str());

        adapterList.push_back(adapter);
        i++;
    }

    for(size_t i = 0; i < adapterList.size(); i++)
    {
        LogAdapterOutputs(adapterList[i]);
        ReleaseCom(adapterList[i]);
    }
}

void D3DApp::LogAdapterOutputs(IDXGIAdapter* adapter)
{
    UINT i = 0;
    IDXGIOutput* output = nullptr;
    while(adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_OUTPUT_DESC desc = {};
        output->GetDesc(&desc);

        std::wstring text = L"***Output: ";
        text += desc.DeviceName;
        text += L"\n";

        OutputDebugStringW(text.c_str());

        LogOutputDisplayModes(output, DXGI_FORMAT_B8G8R8A8_UNORM);

        ReleaseCom(output);
        int* a = nullptr;
        i++;
    }
}

void D3DApp::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
    UINT count = 0;
    UINT flags = 0;

    output->GetDisplayModeList(format, flags, &count, nullptr);

    std::vector<DXGI_MODE_DESC> modeList(count);

    output->GetDisplayModeList(format, flags, &count, &modeList[0]);

    for(size_t i = 0; i < modeList.size(); i++)
    {
        DXGI_MODE_DESC desc = modeList[i];
        std::wstring text =
                L"Width = " + std::to_wstring(desc.Width) + L" " +
                L"Height = " + std::to_wstring(desc.Height) + L" " +
                L"Refresh = " + std::to_wstring(desc.RefreshRate.Numerator) + L"/" + std::to_wstring(desc.RefreshRate.Denominator) +
                L"\n";

        OutputDebugStringW(text.c_str());
    }

    ThrowIfFailed(output->GetDisplayModeList(format, flags, &count, &modeList[0]));
}
