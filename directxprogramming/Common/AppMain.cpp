#include "triangle.h"
#include "../ObjectUtils/objutils.h"
#define D3DCOMPILE_DEBUG 1

static TCHAR szWindowClass[] = _T("App");

static TCHAR szTitle[] = _T("App Title");
HINSTANCE hInst;



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
	UINT width = 800;
	UINT height = 600;
	float aspectRatio =(float) width / height;
	std::vector<Vertex> triangleVertices =
	{
		{{0.0f, 0.25f * aspectRatio, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
		{{0.25f, -0.25f * aspectRatio, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
		{{-0.250f, -0.25f * aspectRatio, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
	};

	//Object stanfordBunny; 
	//stanfordBunny.getObjectFromObj("Objects\\stanford-bunny.obj");
	//DXObject DXstanfordBunny{ stanfordBunny };

	DXObject cube;
	std::vector<DXVertex> vertexBuffer = {
		{{-1,-1,-1,1},{0,0,0}},
		{{-1,1, -1,1},{0,0,0}},
		{{1, 1,-1,1},{0,0,0}},
		{{1, -1, -1,1},{0,0,0}},
		{{ -1,-1,1,1},{0,0,0}},
		{{ -1,+1, 1,1},{0,0,0}},
		{{ 1, 1,1,1},{0,0,0}},
		{{ 1, -1, 1,1},{0,0,0}}
	};
	std::vector<unsigned int> indexBuffer = {
		0, 1, 2,
		0, 2, 3,

		4, 6, 5,
		4, 7, 6,

		4, 5, 1,
		4, 1, 0,

		3, 2, 6,
		3, 6, 7,
		// top face
		1, 5, 6,
		1, 6, 2,
		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	cube.vertexBuffer = vertexBuffer;
	cube.indexBuffer = indexBuffer;
	cube.calcNormalsFromIndexBuffer(3);


	try
	{
		D3DApp win(width, height, aspectRatio);

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
		MSG msg = { 0 };
		win.getTimer()->Reset();
		while (msg.message != WM_QUIT)
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
		}

		win.Destroy();

		return (int)msg.wParam;
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}