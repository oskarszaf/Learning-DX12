#include "cube.h"

void CubeApp::InitDX12()
{
	D3DApp::InitDX12();

	// Reset command list to prep for init commands
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

	BuildDescriptorHeaps();
	BuildConstantBuffers();
	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildGeometry();
	BuildPSO();

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
}

void CubeApp::Update(const GameTimer& gt)
{
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	DirectX::XMVECTOR target = DirectX::XMVectorZero();
	if (m_controlCentered)
	{
		// spherical -> cartesian coordinates
		x = m_radius * sinf(m_phi) * cosf(m_theta);
		y = m_radius * cosf(m_phi);
		z = m_radius * sinf(m_phi) * sinf(m_theta);
		//target = DirectX::XMVectorZero();
	}
	else
	{
		m_translation.x += (m_wpressed ? -0.02f : 0.0f) + (m_spressed ? 0.02f : 0.0f);
		m_translation.z += (m_apressed ? -0.02f : 0.0f) + (m_dpressed ? 0.02f : 0.0f);

		x = 1.0f + m_translation.x;
		y = m_translation.y;
		z = m_translation.z;
		target = DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&m_translation), DirectX::XMVECTOR{ -1.0f,0.0f,0.0f });
		//DirectX::XMVectorSetW(target, 0.0f);
	}
	
	
	// build the view matrix
	DirectX::XMVECTOR pos = DirectX::XMVectorSet(x, y, z, 1.0f);
	
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
	DirectX::XMStoreFloat4x4(&m_view, view);

	DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&m_world);
	DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&m_proj);
	DirectX::XMMATRIX matMVP = DirectX::XMMatrixTranspose(world * view * proj);

	ObjectConstants objConstants;
	DirectX::XMStoreFloat4x4(&objConstants.matMVP, matMVP);
	m_objectCB->CopyData(0, objConstants);
}

// GEOMETRY USED HERE
void CubeApp::Render(const GameTimer& gt)
{
	// reuse memory associated with command recording
	// we can reset only after the associated cmd lists have finished execution on the GPU
	ThrowIfFailed(m_commandAllocator->Reset());

	// cmd list can be reset only after it has been aded to the cmd queue via ExecuteCommandList
	// resuing the cmd list reuses memory
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_PSO.Get()));

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

	auto vbv = m_geometry->VertexBufferView();
	auto ibv = m_geometry->IndexBufferView();
	m_commandList->IASetVertexBuffers(0, 1, &vbv);
	m_commandList->IASetIndexBuffer(&ibv);
	m_commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_commandList->SetGraphicsRootDescriptorTable(0, m_cbvHeap->GetGPUDescriptorHandleForHeapStart());

	//DRAW INDEXED INSTANCED !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	m_commandList->DrawIndexedInstanced(m_geometry->DrawArgs["box"].IndexCount, 1, 0, 0, 0);

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

	// wait until commands are completed
	// THIS IS INEFFICIENT, TO BE FIXED!!!!!!!!
	FlushCommandQueue();
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

void CubeApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap)));
}

void CubeApp::BuildConstantBuffers()
{
	m_objectCB = std::make_unique<UploadBuffer<ObjectConstants>>(m_device.Get(), 1, true);
	
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = m_objectCB->Resource()->GetGPUVirtualAddress();
	// offset to ith object constant buffer in the buffer
	int bufIndex = 0;
	cbAddress += bufIndex * objCBByteSize;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	m_device->CreateConstantBufferView(
		&cbvDesc,
		m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void CubeApp::BuildRootSignature()
{
	// the root signature deines the resources the shader programs expect

	// root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];

	// Create a single descriptor table of CBVs
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);
	
	// A root signature is an array of root parameters
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr,
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
	HRESULT hr = S_OK;

	//m_vsByteCode = d3dUtil::CompileShader(L"Common\\shaders.hlsl", nullptr, "VS", "vs_5_0");
	//m_psByteCode = d3dUtil::CompileShader(L"Common\\shaders.hlsl", nullptr, "PS", "ps_5_0");
	std::wstring filename = L"C:Common\\shaders.hlsl";
	ThrowIfFailed(D3DCompileFromFile(
		filename.c_str(),
		nullptr, nullptr, "VSMain", "vs_5_0",
		0, 0, &m_vsByteCode, nullptr));

	ThrowIfFailed(D3DCompileFromFile(
		filename.c_str(),
		nullptr, nullptr,
		"PSMain", "ps_5_0",
		0, 0, &m_psByteCode, nullptr));

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
	DirectX::XMStoreFloat4x4(&m_world, DirectX::XMMatrixAffineTransformation(DirectX::XMVECTOR{ 10.0f, 10.0f, 10.0f }, DirectX::XMVECTOR{ 0,0,0 }, DirectX::XMVECTOR{ 0,0,0 }, DirectX::XMVECTOR{ 0,-1,0 }));


	const UINT vbByteSize = (UINT)cube.vertexBuffer.size() * sizeof(DXVertex);
	const UINT ibByteSize = (UINT)cube.indexBuffer.size() * sizeof(std::uint16_t);

	m_geometry = std::make_unique<MeshGeometry>();
	m_geometry->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &m_geometry->VertexBufferCPU));
	CopyMemory(m_geometry->VertexBufferCPU->GetBufferPointer(), cube.vertexBuffer.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &m_geometry->IndexBufferCPU));
	CopyMemory(m_geometry->IndexBufferCPU->GetBufferPointer(), cube.indexBuffer.data(), ibByteSize);

	m_geometry->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(m_device.Get(),
		m_commandList.Get(), cube.vertexBuffer.data(), vbByteSize, m_geometry->VertexBufferUploader);

	m_geometry->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_device.Get(),
		m_commandList.Get(), cube.indexBuffer.data(), ibByteSize, m_geometry->IndexBufferUploader);

	m_geometry->VertexByteStride = sizeof(DXVertex);
	m_geometry->VertexBufferByteSize = vbByteSize;
	m_geometry->IndexFormat = DXGI_FORMAT_R16_UINT;
	m_geometry->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)cube.indexBuffer.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	m_geometry->DrawArgs["box"] = submesh;

}

void CubeApp::BuildPSO()
{
	// 4. Create pipeline state object descriptor + create the object
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size()};
	psoDesc.pRootSignature = m_rootSignature.Get();
	//psoDesc.VS = CD3DX12_SHADER_BYTECODE(pVertexShaderData, vertexShaderDataLength);
	//psoDesc.PS = CD3DX12_SHADER_BYTECODE(pPixelShaderData, pixelShaderDataLength);
	psoDesc.VS = { reinterpret_cast<BYTE*>(m_vsByteCode->GetBufferPointer()), m_vsByteCode->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(m_psByteCode->GetBufferPointer()), m_psByteCode->GetBufferSize() };
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PSO)));
}
