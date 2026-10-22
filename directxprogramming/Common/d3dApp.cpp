#include "d3dApp.h"

void GetHardwareAdapter(
	IDXGIFactory1* pFactory,
	IDXGIAdapter1** ppAdapter,
	bool requestHighPerformanceAdapter);

LRESULT D3DApp::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	TCHAR greeting[] = _T("Y");

	switch (message)
	{
	case WM_PAINT:
		Update();
		Render();
		return 0;
	case WM_SIZE:
		m_width = LOWORD(lParam);
		m_height = HIWORD(lParam);
		if (m_device)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				m_appPaused = true;
				m_minimized = true;
				m_maximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				m_appPaused = false;
				m_minimized = false;
				m_maximized = true;
				OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{
				if (m_minimized)
				{
					m_appPaused = false;
					m_minimized = false;
					OnResize();
				}
				else if (m_maximized)
				{
					m_appPaused = false;
					m_maximized = false;
					OnResize();
				}
				else if (m_resizing)
				{

				}
				else
				{
					OnResize();
				}
			}
		}
		return 0;
	
	case WM_ENTERSIZEMOVE:
		m_appPaused = true;
		m_resizing = true;
		m_timer.Stop();
		return 0;

	case WM_EXITSIZEMOVE:
		m_appPaused = false;
		m_resizing = false;
		m_timer.Start();
		OnResize();
		return 0;

	/*case WM_CLOSE:
		if (MessageBox(m_hwnd, L"Really quit?", L"App", MB_OKCANCEL) == IDOK)
		{
			DestroyWindow(m_hwnd);
		}
		return 0;*/
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(m_hwnd, message, wParam, lParam);
		break;
	}
	return TRUE;
}


void D3DApp::InitDX12()
{
	LoadPipeline();
	LoadAssets();
	OnResize();
}

void D3DApp::LoadPipeline()
{
	// 1. Enable debug layer (if debug)

#if defined(DEBUG) || defined(_DEBUG)
	{
		Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
		}
	}
#endif

	// 2. Create device

	Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
	Microsoft::WRL::ComPtr<IDXGIAdapter1> hardwareAdapter;
	// Get first adapter that supports DX12
	// if adapter is nullptr use default adapter
	GetHardwareAdapter(factory.Get(), &hardwareAdapter, true);

	HRESULT hardwareResult = D3D12CreateDevice(
		hardwareAdapter.Get(),
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&m_device));
	if (FAILED(hardwareResult))
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			warpAdapter.Get(), 
			D3D_FEATURE_LEVEL_11_0, 
			IID_PPV_ARGS(&m_device)
		));
	}

	// 9. Create a fence
	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	m_fenceValue = 1;

	//Cache descriptor sizes
	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_DsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_CbvSrvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Check 4x MSAA quality support
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	ThrowIfFailed(m_device->CheckFeatureSupport(
	D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)
	));
	m_4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m_4xMsaaQuality > 0 && "Unexpected MSAA quality level");

	// 3. Describe and create the command queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

	// 7. Create command allocator
	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));

	// 5. Create command list (pipeline state object is nullptr for now)
	ThrowIfFailed(m_device->CreateCommandList(0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_commandAllocator.Get(), nullptr,
		IID_PPV_ARGS(&m_commandList)));

	// 6. Close command list
	ThrowIfFailed(m_commandList->Close());


	// 4. Create swap chain
	CreateSwapChain(factory);
	

	// 5. Create RTV and DSV descriptor heap
	CreateRtvAndDsvDescriptorHeaps();

	
}

void D3DApp::CreateSwapChain(Microsoft::WRL::ComPtr<IDXGIFactory4> factory)
{
	m_swapChain.Reset();

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.BufferDesc.Width = m_width;
	swapChainDesc.BufferDesc.Height = m_height;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
	swapChainDesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality-1) : 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.OutputWindow = m_hwnd;
	swapChainDesc.Windowed = TRUE;

	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
	ThrowIfFailed(factory->CreateSwapChain(
		m_commandQueue.Get(),
		&swapChainDesc,
		&swapChain
	));

	ThrowIfFailed(swapChain.As(&m_swapChain));

	// Fullscreen transitions not supported
	ThrowIfFailed(factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER));

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void D3DApp::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = FrameCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));
}

void D3DApp::OnResize()
{
	assert(m_device);
	assert(m_swapChain);
	assert(m_commandAllocator);

	FlushCommandQueue();

	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

	// Release previous resources
	for (int i = 0; i < FrameCount; ++i)
	{
		m_renderTargets[i].Reset();	
	}
	m_depthStencilBuffer.Reset();

	// Resize swap chain
	ThrowIfFailed(m_swapChain->ResizeBuffers(
		FrameCount,
		m_width, m_height,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
	));
	m_frameIndex = 0;

	// 6. Create frame resources (RTV for each frame)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
		// RTV for each frame
		for (UINT n = 0; n < FrameCount; n++)
		{
			ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
			m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}
	}

	// Create the depth/stencil buffer and view.
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = m_width;
	depthStencilDesc.Height = m_height;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;

	// Correction 11/12/2016: SSAO chapter requires an SRV to the depth buffer to read from 
	// the depth buffer.  Therefore, because we need to create two views to the same resource:
	//   1. SRV format: DXGI_FORMAT_R24_UNORM_X8_TYPELESS
	//   2. DSV Format: DXGI_FORMAT_D24_UNORM_S8_UINT
	// we need to create the depth buffer resource with a typeless format.  
	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	depthStencilDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	ThrowIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(m_depthStencilBuffer.GetAddressOf())
	));

	// Create descriptor to mip level 0 of entire resource using the format of the resource.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.Texture2D.MipSlice = 0;
	m_device->CreateDepthStencilView(
		m_depthStencilBuffer.Get(), 
		&dsvDesc, 
		m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

	// Transition the resource from its initial state to be used as a depth buffer.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_depthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// Execute resize commands
	ThrowIfFailed(m_commandList->Close());
	ID3D12CommandList* cmdsLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until resize is complete.
	FlushCommandQueue();

	// Update viewport transform
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;
	m_viewport.Width = static_cast<float>(m_width);
	m_viewport.Height = static_cast<float>(m_height);
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;

	m_scissorRect = { 0, 0, m_width, m_height };
}

void D3DApp::FlushCommandQueue()
{
	// advance fence
	m_fenceValue++;

	// Add an instruction to the command queue to set a new fence point.  Because we 
	// are on the GPU timeline, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValue));

	if (m_fence->GetCompletedValue() < m_fenceValue)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

		ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValue, eventHandle));

		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void D3DApp::LoadAssets()
{
	// 1. Create empty root signature
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(
			0, nullptr,
			0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		Microsoft::WRL::ComPtr<ID3DBlob> signature;
		Microsoft::WRL::ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&signature, &error));
		ThrowIfFailed(m_device->CreateRootSignature(0,
			signature->GetBufferPointer(), signature->GetBufferSize(),
			IID_PPV_ARGS(&m_rootSignature)));;
	}
	//Create pipeline state
	{
		Microsoft::WRL::ComPtr<ID3DBlob> vertexShader, vsErrors;
		Microsoft::WRL::ComPtr<ID3DBlob> pixelShader, psErrors;
		std::wstring filename = L"C:Common\\shaders.hlsl";
		std::string entrypointVS = "VSMain";
		std::string entrypointPS = "PSMain";
		std::string targetVS = "vs_5_0";
		std::string targetPS = "ps_5_0";

		UINT8* pVertexShaderData = nullptr;
		UINT8* pPixelShaderData = nullptr;
		UINT vertexShaderDataLength = 0;
		UINT pixelShaderDataLength = 0;

		//ThrowIfFailed(ReadDataFromFile(GetAssetFullPath(L"shaders_VSMain.cso").c_str(), &pVertexShaderData, &vertexShaderDataLength));
		//ThrowIfFailed(ReadDataFromFile(GetAssetFullPath(L"shaders_PSMain.cso").c_str(), &pPixelShaderData, &pixelShaderDataLength));

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		// 2. Compile shaders 
		ThrowIfFailed(D3DCompileFromFile(
			filename.c_str(),
			nullptr, nullptr, "VSMain", "vs_5_0",
			compileFlags, 0, &vertexShader, &vsErrors));
		ThrowIfFailed(D3DCompileFromFile(
			filename.c_str(),
			nullptr, nullptr,
			"PSMain", "ps_5_0",
			compileFlags, 0, &pixelShader, &psErrors));

		// 3. Create vertex input layout
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		};

		// 4. Create pipeline state object descriptor + create the object
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_rootSignature.Get();
		//psoDesc.VS = CD3DX12_SHADER_BYTECODE(pVertexShaderData, vertexShaderDataLength);
		//psoDesc.PS = CD3DX12_SHADER_BYTECODE(pPixelShaderData, pixelShaderDataLength);
		psoDesc.VS = { reinterpret_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
		psoDesc.PS = { reinterpret_cast<UINT8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
	}

	

	// 7. Create + load vertex buffers
	{
		Vertex triangleVertices[3] =
		{
			{{0.0f, 0.25f * m_aspectRatio, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
			{{0.25f, -0.25f * m_aspectRatio, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
			{{-0.250f, -0.25f * m_aspectRatio, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
		};

		const UINT vertexBufferSize = sizeof(triangleVertices);

		// Note: using upload heaps to transfer static data like vert buffers is not 
		// recommended. Every time the GPU needs it, the upload heap will be marshalled 
		// over. Please read up on Default Heap usage. An upload heap is used here for 
		// code simplicity and because there are very few verts to actually transfer.
		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		auto desc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
		ThrowIfFailed(m_device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer)
		));

		// Copy triange data to the vertex buffer
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0); // No reading on the CPU
		ThrowIfFailed(m_vertexBuffer->Map(
			0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
		m_vertexBuffer->Unmap(0, nullptr);

		// 8. Create vertex buffer views
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(Vertex);
		m_vertexBufferView.SizeInBytes = vertexBufferSize;
	}

	// Create sync objects and wait until assets have been uploaded to the GPU
	{
		

		// 10. Create event handle
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}
		// 11. Wait for GPU to finish (fence)
		WaitForPreviousFrame();
	}
}

void D3DApp::Update() {}

void D3DApp::Render() 
{
	// Record all commands needed to render the scene into the command list
	PopulateCommandList();

	// Execute command list
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame
	ThrowIfFailed(m_swapChain->Present(1, 0));

	WaitForPreviousFrame();
}

void D3DApp::PopulateCommandList()
{
	//Might be more practical to reset every couple frames

	// Command list allocators can only be reset when the associated 
	// command lists have finished execution on the GPU; apps should use 
	// fences to determine GPU execution progres

	// 1. Reset the command allocator and command list
	ThrowIfFailed(m_commandAllocator->Reset());

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

	// 2. Set the root signature, viewport and scissors rectangles
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	// 3. Indicate back buffer to be used as a render target
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_renderTargets[m_frameIndex].Get(), 
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &barrier);
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// 4. Record the commands
	const float clearColor[] = {0.0f,0.2f, 0.4f, 1.0f};
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->DrawInstanced(3, 1, 0, 0);


	// 5. Indicate the back buffer to be used to present
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_commandList->ResourceBarrier(1, &barrier);

	// 6. Close the command list
	ThrowIfFailed(m_commandList->Close());
}

void D3DApp::WaitForPreviousFrame()
{
	// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
   // This is code implemented as such for simplicity. More advanced samples 
   // illustrate how to use fences for efficient resource usage.

	//Signal and increment fence value
	const UINT64 fence = m_fenceValue;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
	m_fenceValue++;

	//Wait until previous frame is finished
	if (m_fence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void D3DApp::Destroy()
{
	WaitForPreviousFrame();
	CloseHandle(m_fenceEvent);
}

void D3DApp::CalculateFrameStats()
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.

	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// Compute averages over one second period.
	if ((m_timer.TotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		std::wstring fpsStr = std::to_wstring(fps);
		std::wstring mspfStr = std::to_wstring(mspf);
		
		std::wstring windowText = m_windowCaption +
			L"    fps: " + fpsStr +
			L"   mspf: " + mspfStr;

		SetWindowText(m_hwnd, windowText.c_str());

		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

void GetHardwareAdapter(
	IDXGIFactory1* pFactory,
	IDXGIAdapter1** ppAdapter,
	bool requestHighPerformanceAdapter)
{
	*ppAdapter = nullptr;

	Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;

	Microsoft::WRL::ComPtr<IDXGIFactory6> factory6;
	if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
	{
		for (
			UINT adapterIndex = 0;
			SUCCEEDED(factory6->EnumAdapterByGpuPreference(
				adapterIndex,
				requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
				IID_PPV_ARGS(&adapter)));
				++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
	}

	if (adapter.Get() == nullptr)
	{
		for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
	}

	*ppAdapter = adapter.Detach();
}