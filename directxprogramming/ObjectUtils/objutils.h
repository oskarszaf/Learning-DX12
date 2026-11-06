#pragma once
#include <vector>
#include <DirectxMath.h>
#include <string>
#include <iostream>
#include <fstream>

// TODO: OBJECTS AND POLYGON GROUPS
// TODO: MTL SUPPORT

struct FaceElement
{
	std::uint16_t vertex = 0;
	std::uint16_t texcoord  = 0;
	std::uint16_t normal = 0;
};

struct DXVertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT4 color{ 0,0,0,1 };
};


struct Object
{
public:
	std::vector<DirectX::XMFLOAT3> texCoords = {};
	std::vector<DirectX::XMFLOAT3> normals = {};
	std::vector<DirectX::XMFLOAT3> psVertices = {};
	std::vector<std::vector<unsigned int>> lineElements = {};
	

	std::vector<DXVertex> vertexBuffer = {};
	std::vector<std::uint16_t> indexBuffer = {};
	std::vector<std::vector<std::uint16_t>> dxfaces = {};

	void getObjectFromObj(std::string filename)
	{
		std::vector<DirectX::XMFLOAT3> vertices = {};
		std::vector<std::vector<FaceElement>> faces = {};

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
						vertices.push_back(DirectX::XMFLOAT3{ vertex.x, vertex.y, vertex.z });
						break;
					}
					case 4:
					{
						DirectX::XMFLOAT4 vertex = strvecToDXfloat4(subsplit);
						vertices.push_back(DirectX::XMFLOAT3{vertex.x/vertex.w, vertex.y/vertex.w, vertex.z/vertex.w});
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
		}
		else
		{
			std::cout << "Unable to open file." << std::endl;
			return;
		}

		vertexBuffer.resize(vertices.size());
		indexBuffer.resize(3 * faces.size());
		dxfaces.resize(faces.size());

		for (int i = 0; i < vertices.size(); i++)
		{
			DXVertex vertex;
			vertex.pos = vertices.at(i);
			vertexBuffer.at(i) = vertex;
		}
		vertices.clear();
		vertices.shrink_to_fit();

		for (int i = 0; i < faces.size(); i++)
		{
			std::vector<FaceElement> face = faces.at(i);
			indexBuffer.at(3*i) = face.at(0).vertex;
			indexBuffer.at(3 * i + 1) = face.at(1).vertex;
			indexBuffer.at(3 * i + 2) = face.at(2).vertex;
			dxfaces.at(i) = { face.at(0).vertex, face.at(1).vertex , face.at(2).vertex };
		}
		faces.clear();
		faces.shrink_to_fit();

		return;
	}

	void calculateNormalsFromIndexBufferTreatAsColors()
	{
		for (std::vector<std::uint16_t> face : dxfaces)
		{
			DirectX::XMVECTOR a = DirectX::XMLoadFloat3(&vertexBuffer.at(face.at(0)).pos);
			DirectX::XMVECTOR b = DirectX::XMLoadFloat3(&vertexBuffer.at(face.at(1)).pos);
			DirectX::XMVECTOR c = DirectX::XMLoadFloat3(&vertexBuffer.at(face.at(2)).pos);

			DirectX::XMVECTOR e1 = DirectX::XMVectorSubtract(b, a);
			DirectX::XMVECTOR e2 = DirectX::XMVectorSubtract(c, a);

			DirectX::XMVECTOR n = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(e1, e2));
			//printVEC4(n);

			DirectX::XMVECTOR na = DirectX::XMVectorAdd(DirectX::XMLoadFloat4(&vertexBuffer.at(face.at(0)).color), n);
			DirectX::XMStoreFloat4(&vertexBuffer.at(face.at(0)).color, na);

			DirectX::XMVECTOR nb = DirectX::XMVectorAdd(DirectX::XMLoadFloat4(&vertexBuffer.at(face.at(1)).color), n);
			DirectX::XMStoreFloat4(&vertexBuffer.at(face.at(1)).color, nb);

			DirectX::XMVECTOR nc = DirectX::XMVectorAdd(DirectX::XMLoadFloat4(&vertexBuffer.at(face.at(2)).color), n);
			DirectX::XMStoreFloat4(&vertexBuffer.at(face.at(2)).color, nc);

		}

		for (DXVertex v : vertexBuffer)
		{
			DirectX::XMVECTOR vv = DirectX::XMLoadFloat4(&v.color);
			vv = DirectX::XMVector3Normalize(vv);
			//printVEC4(vv);
			DirectX::XMStoreFloat4(&v.color, vv);
		}
	}

	void scaleMesh(float factor)
	{
		for (DXVertex vertex : vertexBuffer)
		{
			//DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&vertex.pos);
			//pos = DirectX::XMVector3Transform(pos, DirectX::XMMatrixScaling(factor, factor, factor));
			//DirectX::XMStoreFloat3(&vertex.pos, pos);
			vertex.pos = DirectX::XMFLOAT3{factor * vertex.pos.x, factor * vertex.pos.y , factor * vertex.pos.z };
		}
	}

	private:
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
				tempf.at(3) };
			return vertex;
		}
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

