#pragma once
#include "d3dApp.h"
#include "UploadBuffer.h"
#include "../ObjectUtils/GeometryGen.h"
#include "Camera.h"
#include "FrameResource.h"



struct RenderItem
{
	RenderItem() = default;

	DirectX::XMFLOAT4X4 Model = MathHelper::Identity4x4();

	int NumFramesDirty = gNumFrameResources;

	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
	UINT ObjCBIndex = -1;

	MeshGeometry* Geo = nullptr;

	// Primitive topology
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	//DrawIndexedInstanced parameters
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
private:
	const int gNumFrameResources = 3;
};

class CubeApp : public D3DApp
{
public:
	CubeApp(UINT width, UINT height, float aspectRatio) 
		: D3DApp(width, height, aspectRatio){}

	virtual void InitDX12()override;
	virtual void Destroy()override;

	void swapControlMode(bool isFreecam)
	{
		camera.switchMode(isFreecam);
	}

private:
	
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Render(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;
	virtual void OnMouseWheelScroll(WPARAM btnState, int x, int y)override;
	virtual void OnKeyDown(WPARAM btnState)override;
	virtual void OnKeyUp(WPARAM btnState)override;

	void UpdateCamera(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);

	void BuildDescriptorHeaps();
	void BuildConstantBufferViews();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();

	void BuildGeometry();
	void BuildPSOs();

	void BuildFrameResources();
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

private:

	Camera camera;

	std::vector<std::unique_ptr<FrameResource>> m_frameResources;
	FrameResource* m_currFrameResource = nullptr;
	int m_currFrameResourceIndex = 0;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbvHeap = nullptr;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> m_geometries;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> m_shaders;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> m_PSOs;

	//std::unique_ptr<UploadBuffer<ObjectConstants>> m_objectCB = nullptr;

	//std::unique_ptr<MeshGeometry> m_geometry = nullptr;

	//Microsoft::WRL::ComPtr<ID3DBlob> m_vsByteCode = nullptr;
	//Microsoft::WRL::ComPtr<ID3DBlob> m_psByteCode = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;

	// List of all RenderItems
	std::vector<std::unique_ptr<RenderItem>> m_renderItems;
	
	// redner items divided by PSO
	std::vector<RenderItem*> m_opaqueRItems;

	PassConstants m_mainPassCB;
	UINT m_passCbvOffset = 0;
	
	bool m_isWireframe = false;

	//Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PSO = nullptr;

	DirectX::XMFLOAT4X4 m_world = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 m_view = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 m_proj = MathHelper::Identity4x4();

	DirectX::XMFLOAT3 m_translation = DirectX::XMFLOAT3{ 0,0,0 };
	bool m_wpressed = false;
	bool m_spressed = false;
	bool m_apressed = false;
	bool m_dpressed = false;

	DirectX::XMFLOAT3 m_eyePos{ 0,0,0 };
	float m_theta = 1.5f * DirectX::XM_PI;
	float m_phi = DirectX::XM_PIDIV4;
	float m_radius = 5.0f;

	POINT m_lastMousePos;
	const int gNumFrameResources = 3;
};



