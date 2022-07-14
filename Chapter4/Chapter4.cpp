// Chapter4.cpp : Defines the entry point for the application.
//

#include "Common/framework.h"
#include "Common/d3dApp.h"
#include "VecAdd.h"
#include "Chapter4.h"
#include <crtdbg.h>

using namespace DirectX;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        VecAdd app(hInstance);
        if(!app.Initialize(D3DApp::D3DAPP_SETTINGS::Default(D3D_FEATURE_LEVEL_11_0, 4)))
        {
            return -1;
        }

        return app.Run();
    }
    catch(DirectXHelper::DxException& e)
    {
        MessageBoxW(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return e.ErrorCode;
    }
}