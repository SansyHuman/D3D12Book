#pragma once

#include "Common/framework.h"
#include "Common/d3dApp.h"

#ifndef D3D12BOOK_INITDIRECT3D_H
#define D3D12BOOK_INITDIRECT3D_H

using namespace DirectX;

class InitDirect3DApp : public D3DApp
{
public:
    InitDirect3DApp(HINSTANCE hInstance);
    ~InitDirect3DApp();

    virtual bool Initialize(const D3DApp::D3DAPP_SETTINGS& settings) override;

private:
    virtual void OnResize() override;
    virtual void Update(GameTimer<float>& gt) override;
    virtual void Draw(const GameTimer<float>& gt) override;
};

InitDirect3DApp::InitDirect3DApp(HINSTANCE hInstance)
    : D3DApp(hInstance)
{
}

InitDirect3DApp::~InitDirect3DApp()
{
}

bool InitDirect3DApp::Initialize(const D3DApp::D3DAPP_SETTINGS& settings)
{
    if(!D3DApp::Initialize(settings))
    {
        return false;
    }

    return true;
}

void InitDirect3DApp::OnResize()
{
    D3DApp::OnResize();
}

void InitDirect3DApp::Update(GameTimer<float>& gt)
{
}

void InitDirect3DApp::Draw(const GameTimer<float>& gt)
{
    ThrowIfFailed(mDirectCmdListAlloc->Reset());

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    D3D12_RESOURCE_BARRIER rtvTarget = CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    mCommandList->ResourceBarrier(1, &rtvTarget);

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CurrentBackBufferView();
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = DepthStencilView();

    mCommandList->ClearRenderTargetView(
        rtvHandle,
        Colors::LightSteelBlue,
        0, nullptr
    );
    mCommandList->ClearDepthStencilView(
        dsvHandle,
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        1.0f, 0, 0, nullptr
    );

    mCommandList->OMSetRenderTargets(1, &rtvHandle, true, &dsvHandle);

    D3D12_RESOURCE_BARRIER rtvPresent = CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );
    mCommandList->ResourceBarrier(1, &rtvPresent);

    ThrowIfFailed(mCommandList->Close());

    ID3D12CommandList* cmdList[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdList), cmdList);

    ThrowIfFailed(mSwapChain->Present(0, 0));
    mCurrentBackBuffer = (mCurrentBackBuffer + 1) % mSwapChainBufferCount;

    FlushCommandQueue();
}

#endif