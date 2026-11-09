#include "cube.h"
#include <random>
#include <chrono>

const int gNumFrameResources = 3;

void CubeApp::InitDX12()
{
	D3DApp::InitDX12();

	// Reset command list to prep for init commands
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

	camera.SetLens(1.0f, 1000.0f, DirectX::XM_PIDIV4, m_aspectRatio);
	camera.LookAt(DirectX::XMFLOAT3{ 0,0,-5 }, DirectX::XMFLOAT3{ 0,0,0 }, DirectX::XMFLOAT3{0,1,0});

	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildGeometry();
	BuildRenderItems();
	BuildFrameResources();
	BuildDescriptorHeaps();
	BuildConstantBufferViews();
	BuildPSOs();

	// Execute init commands
	ThrowIfFailed(m_commandList->Close());
	ID3D12CommandList* cmdLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	// Wait until init complete
	FlushCommandQueue();

}

void CubeApp::Destroy()
{
	D3DApp::Destroy();
}

void CubeApp::OnResize()
{
	D3DApp::OnResize();

	// update aspect ratio, recompute projection matrix
	DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(
		DirectX::XM_PIDIV4, AspectRatio(), 1.0f, 1000.0f);
	DirectX::XMStoreFloat4x4(&m_proj, P);

	camera.SetLens(1.0f, 1000.0f, DirectX::XM_PIDIV4, AspectRatio());
}

void CubeApp::Update(const GameTimer& gt)
{
	UpdateCamera(gt);

	// Cycle through frame resource array
	m_currFrameResourceIndex = (m_currFrameResourceIndex + 1) % gNumFrameResources;
	m_currFrameResource = m_frameResources[m_currFrameResourceIndex].get();

	// Has the GPU finished processing hte commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (m_currFrameResource->Fence != 0 && m_fence->GetCompletedValue() < m_currFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_currFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	UpdateObjectCBs(gt);
	UpdateMainPassCB(gt);
}

// GEOMETRY USED HERE
void CubeApp::Render(const GameTimer& gt)
{
	// reuse memory associated with command recording
	// we can reset only after the associated cmd lists have finished execution on the GPU
	auto cmdListAlloc = m_currFrameResource->CmdListAlloc;
	ThrowIfFailed(cmdListAlloc->Reset());

	// cmd list can be reset only after it has been aded to the cmd queue via ExecuteCommandList
	// resuing the cmd list reuses memory
	if (m_isWireframe)
	{
		ThrowIfFailed(m_commandList->Reset(cmdListAlloc.Get(), m_PSOs["opaque_wireframe"].Get()));
	}
	else
	{
		ThrowIfFailed(m_commandList->Reset(cmdListAlloc.Get(), m_PSOs["opaque"].Get()));
	}
	

	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	// Indicate a state transition on the resource usage;
	auto transition = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &transition);

	// Clear the back and depth buffers
	m_commandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::Black, 0, nullptr);
	m_commandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify buffers to render to
	auto cbbv = CurrentBackBufferView();
	auto dsv = DepthStencilView();
	m_commandList->OMSetRenderTargets(1, &cbbv, true, &dsv);

	ID3D12DescriptorHeap* descriptorHeaps[] = { m_cbvHeap.Get() };
	m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	int passCbvIndex = m_passCbvOffset + m_currFrameResourceIndex;
	auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
	passCbvHandle.Offset(passCbvIndex, m_CbvSrvDescriptorSize);
	m_commandList->SetGraphicsRootDescriptorTable(1, passCbvHandle);

	DrawRenderItems(m_commandList.Get(), m_opaqueRItems);

	// Indicate a state transition on the resource usage
	transition = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_commandList->ResourceBarrier(1, &transition);

	// Done recording commands
	ThrowIfFailed(m_commandList->Close());

	// Add command list to queue for execution
	ID3D12CommandList* cmdLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	// swap back and front buffers
	ThrowIfFailed(m_swapChain->Present(0, 0));
	m_frameIndex = (m_frameIndex + 1) % FrameCount;

	m_currFrameResource->Fence = ++m_fenceValue;

	// Set a new fence point
	m_commandQueue->Signal(m_fence.Get(), m_fenceValue);
}

void CubeApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_lastMousePos.x = x;
	m_lastMousePos.y = y;
	SetCapture(m_hwnd);
}

void CubeApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void CubeApp::OnMouseMove(WPARAM btnState, int x, int y)
{

	if ((btnState & MK_LBUTTON) != 0)
	{
		// each pixel 0.25 degrees
		float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(x - m_lastMousePos.x));
		float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(y - m_lastMousePos.y));


		camera.Rotate(dx, dy);

		// update angles
		m_theta += dx;
		m_phi += dy;

		// restrict m_phi
		m_phi = MathHelper::Clamp(m_phi, 0.1f, DirectX::XM_PI - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0 && m_controlCentered)
	{
		// each pixel 0.005 units
		float dx = 0.005f * static_cast<float>(x - m_lastMousePos.x);
		float dy = 0.005f * static_cast<float>(y - m_lastMousePos.y);

		// update radius
		m_radius += dx - dy;

		// restric the radius
		m_radius = MathHelper::Clamp(m_radius, 1.0f, 15.0f);
	}

	m_lastMousePos.x = x;
	m_lastMousePos.y = y;
}

void CubeApp::OnMouseWheelScroll(WPARAM btnState, int x, int y)
{
	if (m_controlCentered)
	{
		// update radius
		m_radius += 0.001 * GET_WHEEL_DELTA_WPARAM(btnState);

		// restric the radius
		m_radius = MathHelper::Clamp(m_radius, 1.0f, 15.0f);
	}
	else
	{
		// update radius
		m_translation.x += 0.001 * GET_WHEEL_DELTA_WPARAM(btnState);

		// restric the radius
		m_translation.x = MathHelper::Clamp(m_translation.x, 1.0f, 15.0f);
	}
	
}

void CubeApp::OnKeyDown(WPARAM btnState)
{
	if (m_controlCentered) { return; }


	if (btnState == 'W')
	{
		m_wpressed = true;
	}
	else if (btnState == 'S')
	{
		m_spressed = true;
	}
	else if (btnState == 'A')
	{
		m_apressed = true;
	}
	else if (btnState == 'D')
	{
		m_dpressed = true;
	}
}

void CubeApp::OnKeyUp(WPARAM btnState)
{
	if (btnState == 'W')
	{
		m_wpressed = false;
	}
	else if (btnState == 'S')
	{
		m_spressed = false;
	}
	else if (btnState == 'A')
	{
		m_apressed = false;
	}
	else if (btnState == 'D')
	{
		m_dpressed = false;
	}
}

void CubeApp::UpdateCamera(const GameTimer& gt)
{
	if (m_controlCentered)
	{
		// spherical -> cartesian coordinates
		m_eyePos.x = m_radius * sinf(m_phi) * cosf(m_theta);
		m_eyePos.y = m_radius * cosf(m_phi);
		m_eyePos.z = m_radius * sinf(m_phi) * sinf(m_theta);

		// build the view matrix
		DirectX::XMVECTOR pos = DirectX::XMVectorSet(m_eyePos.x, m_eyePos.y, m_eyePos.z, 1.0f);
		DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		DirectX::XMVECTOR target = DirectX::XMVectorZero();
		DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
		DirectX::XMStoreFloat4x4(&m_view, view);
	}
	else
	{
		camera.MoveForward((m_wpressed ? 0.005f : 0.0f) + (m_spressed ? -0.005f : 0.0f));
		camera.MoveRight((m_dpressed ? 0.005f : 0.0f) + (m_apressed ? -0.005f : 0.0f));
	}
	
}

void CubeApp::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = m_currFrameResource->ObjectCB.get();
	for (auto& e : m_renderItems)
	{
		// only update the cbuffer data if the constants have changed.
		// This needs to be tracked per frame resource
		if (e->NumFramesDirty > 0)
		{
			DirectX::XMMATRIX model = XMLoadFloat4x4(&e->Model);

			ObjectConstants objConstants;
			DirectX::XMStoreFloat4x4(&objConstants.Model, DirectX::XMMatrixTranspose(model));

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			e->NumFramesDirty--;
		}
	}
}

void CubeApp::UpdateMainPassCB(const GameTimer& gt)
{
	DirectX::XMMATRIX view = DirectX::XMLoadFloat4x4(&m_view);
	DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&m_proj);

	DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply(view, proj);
	DirectX::XMMATRIX invView = DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(view), view);
	DirectX::XMMATRIX invProj = DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(proj), proj);
	DirectX::XMMATRIX invViewProj = DirectX::XMMatrixInverse(&DirectX::XMMatrixDeterminant(viewProj), viewProj);

	DirectX::XMStoreFloat4x4(&m_mainPassCB.View, DirectX::XMMatrixTranspose(view));
	DirectX::XMStoreFloat4x4(&m_mainPassCB.InvView, DirectX::XMMatrixTranspose(invView));
	DirectX::XMStoreFloat4x4(&m_mainPassCB.Proj, DirectX::XMMatrixTranspose(proj));
	DirectX::XMStoreFloat4x4(&m_mainPassCB.InvProj, DirectX::XMMatrixTranspose(invProj));
	DirectX::XMStoreFloat4x4(&m_mainPassCB.ViewProj, DirectX::XMMatrixTranspose(viewProj));
	DirectX::XMStoreFloat4x4(&m_mainPassCB.InvViewProj, DirectX::XMMatrixTranspose(invViewProj));
	m_mainPassCB.EyePos = m_eyePos;
	m_mainPassCB.RTSize = DirectX::XMFLOAT2((float)m_width, (float)m_height);
	m_mainPassCB.InvRTSize = DirectX::XMFLOAT2(1.0f/m_width, 1.0f/m_height);
	m_mainPassCB.NearZ = 1.0f;
	m_mainPassCB.FarZ = 1000.0f;
	m_mainPassCB.TotalTime = gt.TotalTime();
	m_mainPassCB.DeltaTime = gt.DeltaTime();

	// Debug output
	//OutputDebugStringA(("TotalTime: " + std::to_string(gt.TotalTime()) + "\n").c_str());

	auto currPassCB = m_currFrameResource->PassCB.get();
	currPassCB->CopyData(0, m_mainPassCB);
}

void CubeApp::BuildDescriptorHeaps()
{
	UINT objCount = (UINT)m_renderItems.size();
	
	// Need a CBV descriptor for each object for each frame resource
	// +1 for the perPass CCBV for each frame resource
	UINT numDescriptors = (objCount + 1) * gNumFrameResources;

	// Save an offset to the start of the pass CBVs (the last 3 descriptors)
	m_passCbvOffset = objCount * gNumFrameResources;

	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = numDescriptors;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap)));
}

void CubeApp::BuildConstantBufferViews()
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT objCount = (UINT)m_renderItems.size();

	// Need a CBV descriptor for each object for each frame resource
	for (int frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
	{
		auto objectCB = m_frameResources[frameIndex]->ObjectCB->Resource();
		for (UINT i = 0; i < objCount; ++i)
		{
			//	Offset to ith object constant buffer
			D3D12_GPU_VIRTUAL_ADDRESS cbAddress = objectCB->GetGPUVirtualAddress();
			cbAddress += i * objCBByteSize;

			// offset to the object cbv in the descriptor heap
			int heapIndex = frameIndex * objCount + i;
			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(heapIndex, m_CbvSrvDescriptorSize);

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
			cbvDesc.BufferLocation = cbAddress;
			cbvDesc.SizeInBytes = objCBByteSize;

			m_device->CreateConstantBufferView(&cbvDesc, handle);
		}
	}

	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	for (int frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
	{
		auto passCB = m_frameResources[frameIndex]->PassCB->Resource();
		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = passCB->GetGPUVirtualAddress();

		// offset to the pass cbv in the descriptor heap
		int heapIndex = m_passCbvOffset + frameIndex;
		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(heapIndex, m_CbvSrvDescriptorSize);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = cbAddress;
		cbvDesc.SizeInBytes = passCBByteSize;
		
		m_device->CreateConstantBufferView(&cbvDesc, handle);
	}
}

void CubeApp::BuildRootSignature()
{
	// the root signature deines the resources the shader programs expect

	// root parameter can be a table, root descriptor or root constants.

	// Create 2 single descriptor tables of CBVs
	CD3DX12_DESCRIPTOR_RANGE cbvTable0;
	cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	
	CD3DX12_DESCRIPTOR_RANGE cbvTable1;
	cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
	
	CD3DX12_ROOT_PARAMETER slotRootParameter[2];
	
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
	slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);

	// A root signature is an array of root parameters
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting
	// of a single constant buffer
	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(m_device->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&m_rootSignature)));
}

void CubeApp::BuildShadersAndInputLayout()
{
	std::wstring filename = L"C:Common\\shaders.hlsl";
	Microsoft::WRL::ComPtr<ID3DBlob> vs;
	Microsoft::WRL::ComPtr<ID3DBlob> ps;
	
	ThrowIfFailed(D3DCompileFromFile(
		filename.c_str(),
		nullptr, nullptr, "VSMain", "vs_5_0",
		0, 0, &vs, nullptr));

	ThrowIfFailed(D3DCompileFromFile(
		filename.c_str(),
		nullptr, nullptr,
		"PSMain", "ps_5_0",
		0, 0, &ps, nullptr));

	m_shaders["standardVS"] = vs.Get();
	m_shaders["opaquePS"] = ps.Get();

	//m_shaders["standardVS"] = d3dUtil::CompileShader(filename, nullptr, "VS", "vs_5_1");
	//m_shaders["opaquePS"] = d3dUtil::CompileShader(filename, nullptr, "PS", "ps_5_1");

	m_inputLayout = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};
}

// GEOMETRY BUILT HERE
void CubeApp::BuildGeometry()
{

	Object cube;
	cube.getObjectFromObj("Objects\\stanford-bunny.obj");
	cube.calculateNormalsFromIndexBufferTreatAsColors();	
	//DirectX::XMStoreFloat4x4(&m_world, DirectX::XMMatrixAffineTransformation(DirectX::XMVECTOR{ 10.0f, 10.0f, 10.0f }, DirectX::XMVECTOR{ 0,0,0 }, DirectX::XMVECTOR{ 0,0,0 }, DirectX::XMVECTOR{ 0,-1,0 }));


	const UINT vbByteSize = (UINT)cube.vertexBuffer.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)cube.indexBuffer.size() * sizeof(std::uint16_t);

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)cube.indexBuffer.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "rabbitGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), cube.vertexBuffer.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), cube.indexBuffer.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(m_device.Get(),
		m_commandList.Get(), cube.vertexBuffer.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_device.Get(),
		m_commandList.Get(), cube.indexBuffer.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["rabbit"] = submesh;

	m_geometries[geo->Name] = std::move(geo);


}

void CubeApp::BuildPSOs()
{
	// PSO for opaque objects
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size()};
	opaquePsoDesc.pRootSignature = m_rootSignature.Get();
	//psoDesc.VS = CD3DX12_SHADER_BYTECODE(pVertexShaderData, vertexShaderDataLength);
	//psoDesc.PS = CD3DX12_SHADER_BYTECODE(pPixelShaderData, pixelShaderDataLength);
	opaquePsoDesc.VS = { reinterpret_cast<BYTE*>(m_shaders["standardVS"]->GetBufferPointer()), m_shaders["standardVS"]->GetBufferSize()};
	opaquePsoDesc.PS = { reinterpret_cast<BYTE*>(m_shaders["opaquePS"]->GetBufferPointer()), m_shaders["opaquePS"]->GetBufferSize()};
	opaquePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	opaquePsoDesc.SampleDesc.Count = 1;
	opaquePsoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ThrowIfFailed(m_device->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&m_PSOs["opaque"])));


	// PSO for wireframe objects
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = opaquePsoDesc;
	opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	ThrowIfFailed(m_device->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&m_PSOs["opaque_wireframe"])));
}

void CubeApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; i++)
	{
		m_frameResources.push_back(std::make_unique<FrameResource>(
			m_device.Get(), 1, (UINT)m_renderItems.size()));

	}
}


void CubeApp::BuildRenderItems()
{
	
	for (int i = 0; i < 1000; i++)
	{
		auto bunny = std::make_unique<RenderItem>();
		float roll = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * DirectX::XM_2PI;
		float pitch = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * DirectX::XM_2PI;
		float yaw = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * DirectX::XM_2PI;
		float x = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 50.0f - 25.0f;
		float y = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 50.0f - 25.0f;
		float z = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 50.0f - 25.0f;
		DirectX::XMMATRIX model = DirectX::XMMatrixScaling(8,8,8) 
			* DirectX::XMMatrixRotationRollPitchYaw(roll, pitch, yaw)
			* DirectX::XMMatrixTranslation(x, y, z);
		DirectX::XMStoreFloat4x4(&bunny->Model, model);
		bunny->ObjCBIndex = i;
		bunny->Geo = m_geometries["rabbitGeo"].get();
		bunny->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		bunny->IndexCount = bunny->Geo->DrawArgs["rabbit"].IndexCount;
		bunny->StartIndexLocation = bunny->Geo->DrawArgs["rabbit"].StartIndexLocation;
		bunny->BaseVertexLocation = bunny->Geo->DrawArgs["rabbit"].BaseVertexLocation;
		bunny->NumFramesDirty = gNumFrameResources;

		m_renderItems.push_back(std::move(bunny));
	}

	for (auto& e : m_renderItems)
	{
		m_opaqueRItems.push_back(e.get());
	}
}

void CubeApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	auto objectCB = m_currFrameResource->ObjectCB->Resource();

	for (size_t i = 0; i < ritems.size(); i++)
	{
		auto ri = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView()); 
		cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		// offset to CBV in the descriptor heap for this object and for this frame resource
		UINT cbvIndex = m_currFrameResourceIndex * (UINT)m_opaqueRItems.size() + ri->ObjCBIndex;
		auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
		cbvHandle.Offset(cbvIndex, m_CbvSrvDescriptorSize);

		cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);
		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}