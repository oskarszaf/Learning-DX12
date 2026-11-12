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

	DirectX::XMFLOAT3 v1{ lowx,lowy,lowz };//0
	DirectX::XMFLOAT3 v2{ lowx,hiy, lowz };//1
	DirectX::XMFLOAT3 v3{ hix, hiy, lowz };//2
	DirectX::XMFLOAT3 v4{ hix,lowy, lowz };//3
	DirectX::XMFLOAT3 v5{ lowx, lowy, hiz };//4
	DirectX::XMFLOAT3 v6{ lowx, hiy, hiz };//5
	DirectX::XMFLOAT3 v7{ hix, hiy, hiz };//6
	DirectX::XMFLOAT3 v8{ hix, lowy, hiz };//7

	/*v - 1.0 - 1.0 - 1.0
		v - 1.0 1.0 - 1.0
		v 1.0 1.0 - 1.0
		v 1.0 - 1.0 - 1.0
		v - 1.0 - 1.0 1.0
		v - 1.0 1.0 1.0
		v 1.0 1.0 1.0
		v 1.0 - 1.0 1.0

		f 1 2 3
f 1 3 4
f 5 7 6
f 5 8 7
f 5 6 2
f 5 2 1
f 4 3 7
f 4 7 8
f 2 6 7
f 2 7 3
f 5 1 4
f 5 4 8*/

	box.vertexBuffer = { { v1,{0,0,0,1} },{ v2,{0,0,0,1}},{ v3,{0,0,0,1} },{ v4,{0,0,0,1} },{ v5,{0,0,0,1} },{ v6,{0,0,0,1} },{ v7,{0,0,0,1} },{ v8,{0,0,0,1} } };
	box.indexBuffer = {
	0, 1, 2,
	0, 2, 3,

	4, 6, 5,
	4, 7, 6,

	4, 5, 1,
	4, 1, 0,

	3, 2, 6,
	3, 6, 7,

	1, 5, 6,
	1, 6, 2,

	4, 0, 3,
	4, 3, 7
	};

	box.dxfaces = {
		{0, 1, 2},
		{0, 2, 3},

		{4, 6, 5 },
		{4, 7, 6},

		{4, 5, 1 },
		{4, 1, 0},

		{3, 2, 6 },
		{3, 6, 7},

		{1, 5, 6 },
		{1, 6, 2},

		{4, 0, 3 },
		{4, 3, 7}
	};

	return box;
}

Object GeometryGen::Sphere(float r)
{
	Object sphere;
	return sphere;
}

Object GeometryGen::Elipsoid(float a, float b, float c)
{
	Object sphere;
	return sphere;
}

Object GeometryGen::Cylinder(float r, float h, float n)
{
	return ElipseCylinder(r, r, h, n);
}

Object GeometryGen::ElipseCylinder(float a, float b, float h, float n)
{
	Object elcylinder;
	float dangle = DirectX::XM_2PI / n;

	float angle = 0.0f;
	std::uint16_t index = 0;

	DirectX::XMFLOAT3 O{ 0,0,0 };
	DirectX::XMFLOAT3 O2{ 0,h,0 };
	elcylinder.vertexBuffer.push_back({ O, {0,0,0,1} });
	elcylinder.vertexBuffer.push_back({ O2, {0,0,0,1} });
	index += 2;

	for (int i = 0; i < n; i++)
	{
		DirectX::XMFLOAT3 f{ a * cos(angle),0,b * sin(angle) };
		DirectX::XMFLOAT3 f1{ a * cos(angle + dangle), 0, b * sin(angle + dangle) };

		DirectX::XMFLOAT3 f2{ a * cos(angle),h,b * sin(angle) };
		DirectX::XMFLOAT3 f3{ a * cos(angle + dangle), h, b * sin(angle + dangle) };

		elcylinder.vertexBuffer.push_back({ f, {0,0,0,1} });
		elcylinder.vertexBuffer.push_back({ f1, {0,0,0,1} });
		elcylinder.vertexBuffer.push_back({ f2, {0,0,0,1} });
		elcylinder.vertexBuffer.push_back({ f3, {0,0,0,1} });

		elcylinder.indexBuffer.push_back(index);
		elcylinder.indexBuffer.push_back(static_cast<std::uint16_t>(index + 1));
		elcylinder.indexBuffer.push_back(0);

		elcylinder.indexBuffer.push_back(static_cast<std::uint16_t>(index + 2));
		elcylinder.indexBuffer.push_back(1);
		elcylinder.indexBuffer.push_back(static_cast<std::uint16_t>(index + 3));

		elcylinder.indexBuffer.push_back(index);
		elcylinder.indexBuffer.push_back(static_cast<std::uint16_t>(index + 2));
		elcylinder.indexBuffer.push_back(static_cast<std::uint16_t>(index + 3));

		elcylinder.indexBuffer.push_back(index);
		elcylinder.indexBuffer.push_back(static_cast<std::uint16_t>(index + 3));
		elcylinder.indexBuffer.push_back(static_cast<std::uint16_t>(index + 1));

		elcylinder.dxfaces.push_back({ index, static_cast<std::uint16_t>(index + 1), 0 });
		elcylinder.dxfaces.push_back({ static_cast<std::uint16_t>(index + 2), static_cast<std::uint16_t>(index + 3), 1 });
		elcylinder.dxfaces.push_back({ index, static_cast<std::uint16_t>(index + 2),static_cast<std::uint16_t>(index + 3) });
		elcylinder.dxfaces.push_back({ index,static_cast<std::uint16_t>(index + 3) ,static_cast<std::uint16_t>(index + 1) });


		angle += dangle;
		index += 4;
	}



	return elcylinder;
}

Object GeometryGen::Cone(float r, float h, float n)
{
	return ElipseCone(r, r, h, n);
}

Object GeometryGen::ElipseCone(float a, float b, float h, float n)
{
	Object elcone;
	float dangle = DirectX::XM_2PI / n;

	float angle = 0.0f;
	std::uint16_t index = 0;

	DirectX::XMFLOAT3 O{ 0,0,0 };
	DirectX::XMFLOAT3 O2{ 0,h,0 };
	elcone.vertexBuffer.push_back({ O, {0,0,0,1} });
	elcone.vertexBuffer.push_back({ O2, {0,0,0,1} });
	index+=2;

	for (int i = 0; i < n; i++)
	{
		DirectX::XMFLOAT3 f{ a * cos(angle),0,b * sin(angle) };
		DirectX::XMFLOAT3 f1{ a * cos(angle + dangle), 0, b * sin(angle + dangle) };

		elcone.vertexBuffer.push_back({ f, {0,0,0,1} });
		elcone.vertexBuffer.push_back({ f1, {0,0,0,1} });

		elcone.indexBuffer.push_back(index);
		elcone.indexBuffer.push_back(static_cast<std::uint16_t>(index + 1));
		elcone.indexBuffer.push_back(0);

		elcone.indexBuffer.push_back(static_cast<std::uint16_t>(index));
		elcone.indexBuffer.push_back(1);
		elcone.indexBuffer.push_back(static_cast<std::uint16_t>(index + 1));

		elcone.dxfaces.push_back({ index, static_cast<std::uint16_t>(index + 1), 0 });
		elcone.dxfaces.push_back({ index, static_cast<std::uint16_t>(index + 1), 1 });


		angle += dangle;
		index += 2;
	}



	return elcone;
}
