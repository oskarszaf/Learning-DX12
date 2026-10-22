#pragma once
#include "d3dApp.h"

class TriangleWindow : public D3DApp
{
public:
	TriangleWindow(UINT width, UINT height, float aspectRatio, std::vector<Vertex> vertices) 
		: D3DApp(width, height, aspectRatio)
	{
		std::copy(vertices.begin(), vertices.end(), m_vertices);
	}

	
protected:
	
private:
	Vertex m_vertices[3];
};
