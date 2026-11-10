#define D3DCOMPILE_DEBUG 1
#include "cube.h"
static TCHAR szWindowClass[] = _T("App");

static TCHAR szTitle[] = _T("App Title");
HINSTANCE hInst;



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
	UINT width = 800;
	UINT height = 600;
	float aspectRatio =(float) width / height;

	try
	{
		CubeApp win(width, height, aspectRatio);
		win.swapControlMode(false);

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
		return win.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}