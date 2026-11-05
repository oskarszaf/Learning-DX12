#pragma once
#include "d3dApp.h"
#include "UploadBuffer.h"
//#include "../ObjectUtils/objutils.h"

struct ObjectConstants
{
	DirectX::XMFLOAT4X4 matMVP = MathHelper::Identity4x4();
};

struct DXVertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT4 color;
};

class CubeApp : public D3DApp
{
public:
	CubeApp(UINT width, UINT height, float aspectRatio) 
		: D3DApp(width, height, aspectRatio){}

	virtual void InitDX12()override;
	virtual void Destroy()override;

private:
	
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Render(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	void BuildDescriptorHeaps();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildGeometry();
	void BuildPSO();

private:

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbvHeap = nullptr;

	std::unique_ptr<UploadBuffer<ObjectConstants>> m_objectCB = nullptr;

	std::unique_ptr<MeshGeometry> m_geometry = nullptr;

	Microsoft::WRL::ComPtr<ID3DBlob> m_vsByteCode = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> m_psByteCode = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PSO = nullptr;

	DirectX::XMFLOAT4X4 m_world = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 m_view = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 m_proj = MathHelper::Identity4x4();

	float m_theta = 1.5f * DirectX::XM_PI;
	float m_phi = DirectX::XM_PIDIV4;
	float m_radius = 5.0f;

	POINT m_lastMousePos;
};
