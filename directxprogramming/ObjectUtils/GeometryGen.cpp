#include "GeometryGen.h"

Object GeometryGen::Box(float x, float y, float z)
{
	Object box;
	float lowx = -x / 2.0f;
	float lowy = -y / 2.0f;
	float lowz = -z / 2.0f;
	float hix = x / 2.0f;
	float hiy = y / 2.0f;
	float hiz = z / 2.0f;

	DirectX::XMFLOAT3 v1{lowx,lowy,lowz};
	DirectX::XMFLOAT3 v2{lowx,lowy, hiz};
	DirectX::XMFLOAT3 v3{ lowx, hiy, lowz };
	DirectX::XMFLOAT3 v4{ lowx,hiy, hiz };
	DirectX::XMFLOAT3 v5{hix, lowy, lowz};
	DirectX::XMFLOAT3 v6{ hix, lowy, hiz };
	DirectX::XMFLOAT3 v7{ hix, hiy, lowz };
	DirectX::XMFLOAT3 v8{ hix, hiy, hiz };

	box.vertexBuffer = { { v1 },{ v2 },{ v3 },{ v4 },{ v5 },{ v6 },{ v7 },{ v8 } };


	return box;
}

Object GeometryGen::Sphere(float r)
{

}

Object GeometryGen::Elipsoid(float a, float b, float c)
{

}

Object GeometryGen::Cylinder(float r, float h)
{

}

Object GeometryGen::ElipseCylinder(float a, float b, float h)
{

}

Object GeometryGen::Cone(float r, float h)
{

}

Object GeometryGen::ElipseCone(float a, float b, float h)
{

}

// abc, acd!
void GeometryGen::CreateQuad(Object& object, DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b, DirectX::XMFLOAT3 c, DirectX::XMFLOAT3 d)
{

}

std::vector<DirectX::XMFLOAT3> GeometryGen::CreateTriangle(DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b, DirectX::XMFLOAT3 c)
{

}