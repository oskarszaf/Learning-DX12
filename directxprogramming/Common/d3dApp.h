#pragma once
// Link necessary d3d12 libraries
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"D3D12.lib")
#pragma comment(lib,"dxgi.lib")

#include <windows.h>
#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>

#include "d3dx12.h"

#include <wrl.h>
#include <tchar.h>
#include <vector>
#include <string>
#include <iostream>
#include "d3dUtil.h"

template <class DERIVED_TYPE>
class BaseWindow
{
public:
	static LRESULT CALLBACK WndProc(HWND   hwnd, UINT   message, WPARAM wParam, LPARAM lParam)
	{
		DERIVED_TYPE* pThis = NULL;

		if (message == WM_NCCREATE)
		{
			CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			pThis = (DERIVED_TYPE*)(pCreate->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);

			pThis->m_hwnd = hwnd;
		}
		else
		{
			pThis = (DERIVED_TYPE*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		}

		if (pThis)
		{
			return pThis->HandleMessage(message, wParam, lParam);
		}
		else
		{
			return DefWindowProc(hwnd, message, wParam, lParam);
		}
		return 0;
	}

	BaseWindow() : 
		m_hwnd(NULL) { }

	BOOL Create(
		PCWSTR lpWindowName,
		DWORD dwStyle,
		DWORD dwExStyle = 0,
		int x = CW_USEDEFAULT,
		int y = CW_USEDEFAULT,
		int nWidth = CW_USEDEFAULT,
		int nHeight = CW_USEDEFAULT,
		HWND hWndParent = 0,
		HMENU hMenu = 0
		)
	{
		// Fill out window class


		WNDCLASSEX wc = { 0 };
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = DERIVED_TYPE::WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = GetModuleHandle(NULL);
		wc.hIcon = LoadIcon(wc.hInstance, IDI_APPLICATION);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = ClassName();
		wc.hIconSm = LoadIcon(wc.hInstance, IDI_APPLICATION);

		// Register window class
		if (!RegisterClassEx(&wc))
		{
			MessageBox(NULL, L"Registering class Failed!!!!", L"What", 0);
			return 1;
		}

		// Create window
		HWND hWnd = CreateWindowEx(
			dwExStyle,
			ClassName(),
			lpWindowName,
			dwStyle,
			x, y,
			nWidth, nHeight,
			hWndParent,
			hMenu,
			GetModuleHandle(NULL),
			this
		);

		return m_hwnd ? TRUE: FALSE;
	}

	HWND Window() const { return m_hwnd; }

	protected:

		virtual PCWSTR ClassName() const = 0;
		virtual LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM Param) = 0;
		HWND m_hwnd;
		
};

struct Vertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT4 color;
};

class D3DApp : public BaseWindow<D3DApp>
{
public:
	PCWSTR ClassName() const { return L"D3DApp"; }
	LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);

	D3DApp(UINT width, UINT height, float aspectRatio) :
		m_width(width),
		m_height(height),
		m_aspectRatio(aspectRatio),
		m_frameIndex(0),
		m_viewport{0.0f,0.0f,static_cast<float>(width),static_cast<float>(height),0.0f,1.0f },
		m_scissorRect{ 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) },
		m_rtvDescriptorSize(0),
		m_useWarpDevice(false) {}

	void InitDX12();
	void Update();
	void Render();
	void Destroy();

	

protected:

	static const UINT FrameCount = 2;

	
	
	boolean m_useWarpDevice;
	UINT m_width;
	UINT m_height;
	float m_aspectRatio;
	// Pipeline objects
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
	UINT m_rtvDescriptorSize;

	// App resources
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	// Synchronization objects
	UINT m_frameIndex;
	HANDLE m_fenceEvent;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValue;

	void LoadPipeline();
	void LoadAssets();
	void PopulateCommandList();
	void WaitForPreviousFrame();

	
};