#pragma once
#include <DirectXMath.h>
#include "Windows.h"
#include <string>

class Camera
{
public:
	Camera() 
	{
		SetLens(1.0f, 1000.0f, DirectX::XM_PIDIV4, 1.0f);
	}

	~Camera() = default;

	void SetLens(float nearZ, float farZ, float fovY, float aspect);

	void LookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp);
	void LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& worldUp);

	void Move(float dx, float dy, float dz);
	void Move(DirectX::XMFLOAT3 d);
	void MoveForward(float d);
	void MoveRight(float d);
	void MoveUp(float d);

	void Rotate(float pitch, float yaw);
	void RotateX(float pitch);
	void RotateY(float yaw);

	DirectX::XMMATRIX getView() const
	{
		return XMLoadFloat4x4(&m_view);
	}

	void setView(DirectX::FXMMATRIX view)
	{
		DirectX::XMStoreFloat4x4(&m_view, view);
	}

	DirectX::XMMATRIX getProj() const
	{
		return XMLoadFloat4x4(&m_proj);
	}

	void setProj(DirectX::FXMMATRIX proj)
	{
		DirectX::XMStoreFloat4x4(&m_proj, proj);
	}

	void switchMode(bool isFreecam)
	{
		m_freecam = isFreecam;
		
		if (!m_freecam)
		{
			DirectX::XMStoreFloat3(&pivot, DirectX::XMVectorMultiplyAdd(DirectX::XMVectorReplicate(m_radius), DirectX::XMLoadFloat3(&m_look), DirectX::XMLoadFloat3(&m_pos)));
			OutputDebugStringA(std::string("LALALA").c_str());
		}
	}

	bool isFreecam()
	{
		return m_freecam;
	}

private:
	DirectX::XMFLOAT4X4 m_view;
	DirectX::XMFLOAT4X4 m_proj;

	DirectX::XMFLOAT3 m_pos{0,0,0};
	DirectX::XMFLOAT3 m_look{0,0,1};
	DirectX::XMFLOAT3 m_right{1,0,0};
	DirectX::XMFLOAT3 m_up{0,1,0};
	DirectX::XMFLOAT3 m_worldUp{ 0,1,0 };

	float m_nearz = 0.0f;
	float m_farz = 0.0f;
	float m_aspect = 0.0f;
	float m_fovy = 0.0f;
	float m_nearwinheight = 0.0f;
	float m_farwinheight = 0.0f;

	bool m_freecam = false;

	float m_radius = 5.0f;
	float m_phi = DirectX::XM_PIDIV4;
	DirectX::XMFLOAT3 pivot{ 0,0,0 };

};
