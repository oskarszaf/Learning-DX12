#pragma once
#include <vector>
#include <DirectxMath.h>
#include <string>
#include <iostream>
#include <fstream>

// TODO: OBJECTS AND POLYGON GROUPS
// TODO: MTL SUPPORT
std::vector<std::string> splitstr(std::string str, char d);
DirectX::XMFLOAT3 strvecToDXfloat3(std::vector<std::string> strvec);
DirectX::XMFLOAT4 strvecToDXfloat4(std::vector<std::string> strvec);

struct FaceElement
{
	unsigned int vertex = 0;
	unsigned int texcoord = 0;
	unsigned int normal = 0;
};

struct Object
{
	std::vector<DirectX::XMFLOAT4> vertices = {};
	std::vector<DirectX::XMFLOAT3> texCoords = {};
	std::vector<DirectX::XMFLOAT3> normals = {};
	std::vector<DirectX::XMFLOAT3> psVertices = {};
	std::vector<std::vector<unsigned int>> lineElements = {};
	std::vector<std::vector<FaceElement>> faces = {};

	void getObjectFromObj(std::string filename)
	{
		std::ifstream f;
		int linecount = 0;
		f.open(filename);
		if (f.is_open())
		{
			std::string line;
			while (getline(f, line))
			{
				linecount++;
				if (line.length() == 0) { continue; }

				std::vector<std::string> split = splitstr(line, ' ');
				if (split.at(0) == "v")
				{

					std::vector<std::string> subsplit(split.begin() + 1, split.end());
					switch (subsplit.size())
					{
					case 3:
					{
						DirectX::XMFLOAT3 vertex = strvecToDXfloat3(subsplit);
						vertices.push_back(DirectX::XMFLOAT4{ vertex.x, vertex.y, vertex.z, 1.0 });
						break;
					}
					case 4:
					{
						DirectX::XMFLOAT4 vertex = strvecToDXfloat4(subsplit);
						vertices.push_back(vertex);
						break;
					}
					default:
						std::cout << "boom" << std::endl;
					}
				}
				else if (split.at(0) == "vt")
				{
					std::vector<std::string> subsplit(split.begin() + 1, split.end());
					DirectX::XMFLOAT3 texCoord{};
					switch (subsplit.size())
					{
					case 1:
					{
						texCoord.x = stof(subsplit.at(0));
						texCoord.y = 0.0; texCoord.z = 0.0;
					}
					case 2:
					{
						texCoord.x = stof(subsplit.at(0));
						texCoord.y = stof(subsplit.at(1));
						texCoord.z = 0.0;
					}
					case 3:
					{
						texCoord.x = stof(subsplit.at(0));
						texCoord.y = stof(subsplit.at(1));
						texCoord.z = stof(subsplit.at(2));
					}
					}
					texCoords.push_back(texCoord);
				}
				else if (split.at(0) == "vn")
				{
					std::vector<std::string> subsplit(split.begin() + 1, split.end());
					DirectX::XMFLOAT3 normal{ stof(subsplit.at(0)), stof(subsplit.at(1)), stof(subsplit.at(2)) };
					normals.push_back(normal);
				}
				else if (split.at(0) == "vp")
				{
					std::vector<std::string> subsplit(split.begin() + 1, split.end());
					DirectX::XMFLOAT3 psVertex{};
					switch (subsplit.size())
					{
					case 1:
					{
						psVertex.x = stof(subsplit.at(0));
						psVertex.y = 0.0; psVertex.z = 0.0;
					}
					case 2:
					{
						psVertex.x = stof(subsplit.at(0));
						psVertex.y = stof(subsplit.at(1));
						psVertex.z = 0.0;
					}
					case 3:
					{
						psVertex.x = stof(subsplit.at(0));
						psVertex.y = stof(subsplit.at(1));
						psVertex.z = stof(subsplit.at(2));
					}
					}
					psVertices.push_back(psVertex);
				}
				else if (split.at(0) == "f")
				{
					std::vector<FaceElement> face;
					std::vector<std::string> subsplit(split.begin() + 1, split.end());
					for (std::string el : subsplit)
					{
						FaceElement element = {};
						std::vector<std::string> elsplit = splitstr(el, '/');
						switch (elsplit.size())
						{
						case 1:
						{
							element.vertex = (unsigned int)stoi(elsplit.at(0)) - 1;
							break;
						}
						case 2:
						{
							element.vertex = (unsigned int)stoi(elsplit.at(0)) - 1;
							element.normal = (unsigned int)stoi(elsplit.at(1)) - 1;
							break;
						}
						case 3:
						{
							element.vertex = (unsigned int)stoi(elsplit.at(0)) - 1;
							element.texcoord = (unsigned int)stoi(elsplit.at(1)) - 1;
							element.normal = (unsigned int)stoi(elsplit.at(2)) - 1;
							break;
						}
						}
						face.push_back(element);
					}
					faces.push_back(face);
				}
				else if (split.at(0) == "l")
				{
					std::vector<std::string> subsplit(split.begin() + 1, split.end());
					std::vector<unsigned int> line;
					for (std::string v : subsplit)
					{
						line.push_back((unsigned int)stoi(v) - 1);
					}
					lineElements.push_back(line);
				}
				else
				{
					continue;
				}
			}
			std::cout << linecount << std::endl;
			f.close();
			std::cout << vertices.size() << std::endl;

			return;
		}
		else
		{
			std::cout << "Unable to open file." << std::endl;
		}
		return;
	}


};

struct DXVertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT4 normal{ 0,0,0,0 };
};

struct DXObject
{
	std::vector<DXVertex> vertexBuffer;
	std::vector<unsigned int> indexBuffer;
	std::vector<std::vector<unsigned int>> faces;

	/*DXObject(Object& object)
	{
		std::vector<DXVertex> vertvec;
		for (DirectX::XMFLOAT4 position: object.vertices)
		{
			DXVertex vertex;
			vertex.pos = position;
			vertvec.push_back(vertex);
		}
		calculateNormals(object, vertvec);
		vertexBuffer = vertvec;
		std::cout << vertexBuffer[38000].normal.x << " " << vertexBuffer[38000].normal.y <<" "<< vertexBuffer[38000].normal.z << std::endl;
	}*/

	DXObject() {}
	/*
	void calculateNormals(Object& object, std::vector<DXVertex>& vertvec)
	{
		std::vector<unsigned int> indices;
		for (std::vector<FaceElement> face : object.faces)
		{
			unsigned int i0 = face.at(0).vertex;
			unsigned int i1 = face.at(1).vertex;
			unsigned int i2 = face.at(2).vertex;
			indices.push_back(i0);
			indices.push_back(i1);
			indices.push_back(i2);

			DirectX::XMFLOAT4* pa = &object.vertices.at(i0);
			DirectX::XMFLOAT4* pb = &object.vertices.at(i1);
			DirectX::XMFLOAT4* pc = &object.vertices.at(i2);

			DirectX::XMVECTOR a = cart(DirectX::XMLoadFloat4(pa));
			DirectX::XMVECTOR b = cart(DirectX::XMLoadFloat4(pb));
			DirectX::XMVECTOR c = cart(DirectX::XMLoadFloat4(pc));

			DirectX::XMVECTOR e1 = DirectX::XMVectorSubtract(b, a);
			DirectX::XMVECTOR e2 = DirectX::XMVectorSubtract(c, a);

			DirectX::XMVECTOR n = DirectX::XMVector3Cross(e1, e2);

			DirectX::XMVECTOR n0 = DirectX::XMVectorAdd(DirectX::XMLoadFloat4(&vertvec.at(i0).normal), n);
			XMStoreFloat4(&vertvec.at(i0).normal, n0);

			DirectX::XMVECTOR n1 = DirectX::XMVectorAdd(DirectX::XMLoadFloat4(&vertvec.at(i1).normal), n);
			XMStoreFloat4(&vertvec.at(i1).normal, n1);

			DirectX::XMVECTOR n2 = DirectX::XMVectorAdd(DirectX::XMLoadFloat4(&vertvec.at(i2).normal), n);
			XMStoreFloat4(&vertvec.at(i1).normal, n2);
		}
		for (DXVertex v : vertvec)
		{
			DirectX::XMVECTOR vv = DirectX::XMLoadFloat3(&v.normal);
			vv = DirectX::XMVector3Normalize(vv);
			DirectX::XMStoreFloat3(&v.normal, vv);
		}
		indexBuffer = indices;
	}

	void calcNormalsFromIndexBuffer(unsigned int dface)
	{
		calculateFaces(dface);

		for (std::vector<unsigned int> face : faces)
		{
			DirectX::XMVECTOR a = DirectX::XMLoadFloat3(&vertexBuffer.at(face.at(0)).pos);
			DirectX::XMVECTOR b = DirectX::XMLoadFloat3(&vertexBuffer.at(face.at(1)).pos);
			DirectX::XMVECTOR c = DirectX::XMLoadFloat3(&vertexBuffer.at(face.at(2)).pos);

			DirectX::XMVECTOR e1 = DirectX::XMVectorSubtract(b, a);
			DirectX::XMVECTOR e2 = DirectX::XMVectorSubtract(c, a);

			DirectX::XMVECTOR n = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(e1, e2));
			printVEC4(n);

			DirectX::XMVECTOR na = DirectX::XMVectorAdd(DirectX::XMLoadFloat4(&vertexBuffer.at(face.at(0)).normal), n);
			DirectX::XMStoreFloat4(&vertexBuffer.at(face.at(0)).normal, na);

			DirectX::XMVECTOR nb = DirectX::XMVectorAdd(DirectX::XMLoadFloat4(&vertexBuffer.at(face.at(1)).normal), n);
			DirectX::XMStoreFloat4(&vertexBuffer.at(face.at(1)).normal, nb);

			DirectX::XMVECTOR nc = DirectX::XMVectorAdd(DirectX::XMLoadFloat4(&vertexBuffer.at(face.at(2)).normal), n);
			DirectX::XMStoreFloat4(&vertexBuffer.at(face.at(2)).normal, nc);

		}

		for (DXVertex v : vertexBuffer)
		{
			DirectX::XMVECTOR vv = DirectX::XMLoadFloat4(&v.normal);
			vv = DirectX::XMVector3Normalize(vv);
			printVEC4(vv);
			DirectX::XMStoreFloat4(&v.normal, vv);
		}
		
	}

	void calculateFaces(unsigned int dface)
	{
		for (unsigned int i = 0; i < indexBuffer.size() - dface + 1; i += dface)
		{
			std::vector<unsigned int> face;
			for (unsigned int j = i; j < i + dface; j++)
			{
				face.push_back(indexBuffer.at(j));
			}
			faces.push_back(face);
		}
	}

	DirectX::XMVECTOR cart(DirectX::FXMVECTOR hom)
	{
		DirectX::XMFLOAT3 c{ DirectX::XMVectorGetX(hom) / DirectX::XMVectorGetW(hom),
			DirectX::XMVectorGetY(hom) / DirectX::XMVectorGetW(hom),
			DirectX::XMVectorGetZ(hom) / DirectX::XMVectorGetW(hom) };
		DirectX::XMVECTOR c1 = DirectX::XMLoadFloat3(&c);
		return c1;
	}

	void printVEC4(DirectX::FXMVECTOR v)
	{
		std::cout << DirectX::XMVectorGetX(v) << " " << DirectX::XMVectorGetY(v) << " " << DirectX::XMVectorGetZ(v) << " " << DirectX::XMVectorGetW(v) << std::endl;
	}*/
};



/*struct Material
{
	std::string name = "";
	float specEx = 1.0;
	float alpha = 1.0;
	float refindex = 1.0;
	DirectX::XMFLOAT3 ambient = { 0,0,0 };
	DirectX::XMFLOAT3 diffuse = { 0,0,0 };
	DirectX::XMFLOAT3 specular = { 0,0,0 };
	DirectX::XMFLOAT3 transmissionFilter = { 1.0,1.0,1.0 };
	
};*/





std::vector<std::string> splitstr(std::string str, char d)
{
	std::string strt = str;
	std::vector<std::string> split;
	int space;
	while ((space = strt.find_first_of(d)) != std::string::npos)
	{
		split.push_back(strt.substr(0, space));
		strt = strt.substr(space + 1);
	}
	split.push_back(strt);
	return split;
}

DirectX::XMFLOAT3 strvecToDXfloat3(std::vector<std::string> strvec)
{	
	std::vector<float> tempf;
	for (int i = 0; i < strvec.size(); i++)
	{
		tempf.push_back(stof(strvec.at(i)));
	}
	if (tempf.size() < 3) { std::cout << "Error parsing" << std::endl; DirectX::XMFLOAT3 fvec{}; return fvec; }
	DirectX::XMFLOAT3 vertex{
		tempf.at(0),
		tempf.at(1),
		tempf.at(2) };
	return vertex;
}

DirectX::XMFLOAT4 strvecToDXfloat4(std::vector<std::string> strvec)
{
	std::vector<float> tempf;
	for (int i = 0; i < strvec.size(); i++)
	{
		tempf.push_back(stof(strvec.at(i)));
	}
	if (tempf.size() < 4) { std::cout << "Error parsing" << std::endl; DirectX::XMFLOAT4 fvec{}; return fvec; }
	DirectX::XMFLOAT4 vertex{
		tempf.at(0),
		tempf.at(1),
		tempf.at(2), 
		tempf.at(3)};
	return vertex;
}

