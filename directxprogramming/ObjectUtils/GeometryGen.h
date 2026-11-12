#pragma once
#include "objutils.h"

class GeometryGen
{
public:
	Object Box(float x, float y, float z);

	Object Sphere(float r);
	Object Elipsoid(float a, float b, float c);

	Object Cylinder(float r, float h);
	Object ElipseCylinder(float a, float b, float h);

	Object Cone(float r, float h);
	Object ElipseCone(float a, float b, float h);

private:
	void CreateQuad(Object& object, DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b, DirectX::XMFLOAT3 c, DirectX::XMFLOAT3 d); // abc, acd!
	std::vector<DirectX::XMFLOAT3> CreateTriangle(DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b, DirectX::XMFLOAT3 c);
};