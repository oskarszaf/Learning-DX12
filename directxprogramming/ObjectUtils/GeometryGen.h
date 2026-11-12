#pragma once
#include "objutils.h"

class GeometryGen
{
public:
	Object Box(float x, float y, float z);

	Object Sphere(float r);
	Object Elipsoid(float a, float b, float c);

	Object Cylinder(float r, float h, float n);
	Object ElipseCylinder(float a, float b, float h, float n);

	Object Cone(float r, float h, float n);
	Object ElipseCone(float a, float b, float h, float n);
};