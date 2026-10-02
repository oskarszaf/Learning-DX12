#include "d3dApp.h"
#define D3DCOMPILE_DEBUG 1

static TCHAR szWindowClass[] = _T("App");

static TCHAR szTitle[] = _T("App Title");
HINSTANCE hInst;



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
	UINT width = 800;
	UINT height = 600;
	float aspectRatio =(float) width / height;
	D3DApp win(width,height,aspectRatio);

	if (!win.Create(
		L"Yeah", WS_OVERLAPPEDWINDOW, 0, CW_USEDEFAULT, CW_USEDEFAULT, width, height))
	{
		MessageBox(NULL, L"Creating window failed", L"What", 0);
		return 0;
	}

	hInst = hInstance;
	win.InitDX12();
	
	// Show window
	ShowWindow(win.Window(), nCmdShow);

	//Message loop
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	win.Destroy();

	return (int)msg.wParam;
}