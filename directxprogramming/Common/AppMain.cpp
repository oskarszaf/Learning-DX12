#include "cube.h"

#define D3DCOMPILE_DEBUG 1

static TCHAR szWindowClass[] = _T("App");

static TCHAR szTitle[] = _T("App Title");
HINSTANCE hInst;



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
	UINT width = 800;
	UINT height = 600;
	float aspectRatio =(float) width / height;

	//Object stanfordBunny; 
	//stanfordBunny.getObjectFromObj("Objects\\stanford-bunny.obj");
	//DXObject DXstanfordBunny{ stanfordBunny };

	//DXObject cube;

	//cube.vertexBuffer = vertexBuffer;
	//cube.indexBuffer = indexBuffer;
	//cube.calcNormalsFromIndexBuffer(3);


	try
	{
		CubeApp win(width, height, aspectRatio);

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
		//MSG msg = { 0 };
		//win.getTimer()->Reset();
		/*while(msg.message != WM_QUIT)
		{
			if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				win.getTimer()->Tick();
				if (!win.paused())
				{
					win.CalculateFrameStats();
					win.Update();
					win.Render();
				}
				else
				{
					Sleep(100);
				}
			}
		}*/
		//int ret = win.Run();
		//win.Destroy();

		return win.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}