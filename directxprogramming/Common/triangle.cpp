#include "triangle.h"

	void TriangleWindow::LoadAssets()
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

		// 5. Create command list
		ThrowIfFailed(m_device->CreateCommandList(0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_commandAllocator.Get(), m_pipelineState.Get(),
			IID_PPV_ARGS(&m_commandList)));

		// 6. Close command list
		ThrowIfFailed(m_commandList->Close());

		// 7. Create + load vertex buffers
		{

			const UINT vertexBufferSize = sizeof(m_vertices);

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
			memcpy(pVertexDataBegin, m_vertices, sizeof(m_vertices));
			m_vertexBuffer->Unmap(0, nullptr);

			// 8. Create vertex buffer views
			m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
			m_vertexBufferView.StrideInBytes = sizeof(Vertex);
			m_vertexBufferView.SizeInBytes = vertexBufferSize;
		}

		// Create sync objects and wait until assets have been uploaded to the GPU
		{
			// 9. Create a fence
			ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
			m_fenceValue = 1;

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
