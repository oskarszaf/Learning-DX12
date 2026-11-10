#include "Camera.h"
#include <algorithm>
using namespace DirectX;

void Camera::SetLens(float nearZ, float farZ, float fovY, float aspect)
{
	m_nearz = nearZ;
	m_farz = farZ;
	m_aspect = aspect;
	m_fovy = fovY;
	m_nearwinheight = 2.0f * m_nearz * tan(0.5f*fovY);
	m_farwinheight = 2.0f * m_farz * tan(0.5f * fovY);

	XMMATRIX proj = XMMatrixPerspectiveFovLH(m_fovy, m_aspect, m_nearz, m_farz);
	XMStoreFloat4x4(&m_proj, proj);
}

void Camera::LookAt(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR worldUp)
{
	XMVECTOR look = XMVector3Normalize(XMVectorSubtract(target, pos));
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(worldUp, look));
	XMVECTOR up = XMVector3Normalize(XMVector3Cross(look, right));

	XMStoreFloat3(&m_pos, pos);
	XMStoreFloat3(&m_look, look);
	XMStoreFloat3(&m_right, right);
	XMStoreFloat3(&m_up, up);
	XMStoreFloat3(&m_worldUp, worldUp);

	XMMATRIX view = XMMatrixLookAtLH(pos, XMVectorAdd(look, pos), up);
	XMStoreFloat4x4(&m_view, view);
}
void Camera::LookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& worldUp)
{
	XMVECTOR p = XMLoadFloat3(&pos);
	XMVECTOR t = XMLoadFloat3(&target);
	XMVECTOR u = XMLoadFloat3(&worldUp);

	LookAt(p, t, u);
}

void Camera::Move(float dForward, float dRight, float dUp)
{
	XMVECTOR L = XMLoadFloat3(&m_look);
	XMVECTOR R = XMLoadFloat3(&m_right);
	XMVECTOR U = XMLoadFloat3(&m_up);
	XMVECTOR P = XMLoadFloat3(&m_pos);

	XMVECTOR f = XMVectorReplicate(dForward);
	XMVECTOR r = XMVectorReplicate(dRight);
	XMVECTOR u = XMVectorReplicate(dUp);

	P = XMVectorMultiplyAdd(f, L, P);
	P = XMVectorMultiplyAdd(r, R, P);
	P = XMVectorMultiplyAdd(u, U, P);

	XMMATRIX view = XMMatrixLookAtLH(P, L, U);
	XMStoreFloat4x4(&m_view, view);
	XMStoreFloat3(&m_pos, P);

}
void Camera::Move(XMFLOAT3 d)
{
	Move(d.x, d.y, d.z);
}
void Camera::MoveForward(float d)
{
	XMVECTOR P = XMLoadFloat3(&m_pos);
	XMVECTOR L = XMLoadFloat3(&m_look);
	XMVECTOR U = XMLoadFloat3(&m_up);
	if (m_freecam)
	{
		XMVECTOR f = XMVectorReplicate(d);
		P = XMVectorMultiplyAdd(f, L, P);
	}
	else
	{ 
		float prevR = m_radius;
		m_radius -= d; std::max(1.0f, m_radius); 
		float diff = prevR - m_radius;
		XMVECTOR f = XMVectorReplicate(diff);
		P = XMVectorMultiplyAdd(f, L, P);

	}

	
	XMMATRIX view = XMMatrixLookAtLH(P, XMVectorAdd(L, P), U);

	XMStoreFloat4x4(&m_view, view);
	XMStoreFloat3(&m_pos, P);
}
void Camera::MoveRight(float d)
{
	XMVECTOR L = XMLoadFloat3(&m_look);
	XMVECTOR P = XMLoadFloat3(&m_pos);
	XMVECTOR U = XMLoadFloat3(&m_up);
	XMVECTOR R = XMLoadFloat3(&m_right);
	

	if (m_freecam)
	{
		XMVECTOR r = XMVectorReplicate(d);
		P = XMVectorMultiplyAdd(r, R, P);
	}
	else
	{
		XMVECTOR radVec = XMVectorReplicate(m_radius);
		XMVECTOR Center = XMVectorMultiplyAdd(radVec, R, P);
		XMMATRIX M = XMMatrixTranslationFromVector(-Center)*XMMatrixRotationRollPitchYaw(0, 0, d) * XMMatrixTranslationFromVector(Center);
		P = XMVector3TransformNormal(P, M);
	}

	
	XMMATRIX view = XMMatrixLookAtLH(P, XMVectorAdd(L, P), U);

	XMStoreFloat4x4(&m_view, view);
	XMStoreFloat3(&m_pos, P);
}
void Camera::MoveUp(float d)
{
	XMVECTOR R = XMLoadFloat3(&m_right);
	XMVECTOR P = XMLoadFloat3(&m_pos);
	XMVECTOR U = XMLoadFloat3(&m_up);
	XMVECTOR L = XMLoadFloat3(&m_look);
	

	if (m_freecam)
	{
		XMVECTOR u = XMVectorReplicate(d);
		P = XMVectorMultiplyAdd(u, U, P);
	}
	else
	{
		XMVECTOR radVec = XMVectorReplicate(m_radius);
		XMVECTOR Center = XMVectorMultiplyAdd(radVec, U, P);
		XMMATRIX M = XMMatrixTranslationFromVector(-Center) * XMMatrixRotationRollPitchYaw(0, d, 0) * XMMatrixTranslationFromVector(Center);
		P = XMVector3TransformNormal(P, M);
		R = XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&m_worldUp), L));
		U = XMVector3Normalize(XMVector3Cross(L, R));
		XMStoreFloat3(&m_right, R);
		XMStoreFloat3(&m_up, U);
	}

	
	XMMATRIX view = XMMatrixLookAtLH(P, L, U);

	XMStoreFloat4x4(&m_view, view);
	XMStoreFloat3(&m_pos, P);
}

void Camera::Rotate(float pitch, float yaw)
{
	XMMATRIX Rotation = XMMatrixRotationRollPitchYaw(0, pitch, yaw);

	XMVECTOR L = XMLoadFloat3(&m_look);
	L = XMVector3TransformNormal(L, Rotation);

	XMVECTOR R = XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&m_worldUp), L));
	XMVECTOR U = XMVector3Normalize(XMVector3Cross(L, R));

	XMMATRIX view = XMMatrixLookAtLH(XMLoadFloat3(&m_pos), XMVectorAdd(L, XMLoadFloat3(&m_pos)), U);
	XMStoreFloat4x4(&m_view, view);

	XMStoreFloat3(&m_look, L);
	XMStoreFloat3(&m_right, R);
	XMStoreFloat3(&m_up, U);
}
void Camera::RotateX(float pitch)
{

}
void Camera::RotateY(float yaw)
{

}