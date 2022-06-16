#pragma once
#include <d3d12.h>
#include <cassert>
#include <DirectXMath.h>
#include <DirectXTex.h>
using namespace DirectX;

class Buffer
{
protected:
	void Init();
public:
	D3D12_RESOURCE_DESC resDesc;
	ID3D12Resource* buff;

	void SetResource(size_t width, size_t height, D3D12_RESOURCE_DIMENSION Dimension);
	void CreateBuffer(ID3D12Device* device, D3D12_HEAP_PROPERTIES heapProp);
};

class ConstBuf :public Buffer
{
private:
	struct ConstBufferDataMaterial { XMFLOAT4 color; };
	struct ConstBufferDataTransform { XMMATRIX mat; };

	int type;
public:
	enum Type { Material, Transform };
	UINT size;

	ConstBufferDataMaterial* mapMaterial;
	ConstBufferDataTransform* mapTransform;

	ConstBuf(Type type);
	void Mapping();
};


class VertexBuf :public Buffer
{
public:
	struct Vertex
	{
		XMFLOAT3 pos;
		XMFLOAT2 uv;
	};
private:
	Vertex* map;
public:

	D3D12_VERTEX_BUFFER_VIEW view;
	UINT size;

	VertexBuf(UINT size);
	void Mapping(Vertex* vertices, const int ARRAY_NUM);
	void CreateView();
};

class IndexBuf :public Buffer
{
private:
	uint16_t* map;
public:
	D3D12_INDEX_BUFFER_VIEW view;
	UINT size;

	IndexBuf(UINT size);
	void Mapping(uint16_t* indices, const int ARRAY_NUM);
	void CreateView();
};

class TextureBuf :public Buffer
{
	TexMetadata metadata;
	ScratchImage scratchImg;
	ScratchImage mipChain;
public:
	D3D12_SHADER_RESOURCE_VIEW_DESC view;

	TextureBuf();
	void SetResource()
	{
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resDesc.Format = metadata.format;
		resDesc.Width = metadata.width;
		resDesc.Height = (UINT)metadata.height;
		resDesc.DepthOrArraySize = (UINT16)metadata.arraySize;
		resDesc.MipLevels = (UINT16)metadata.mipLevels;
		resDesc.SampleDesc.Count = 1;
	}
	void LoadTexture()
	{
		LoadFromWICFile(L"Resources/Map.png", WIC_FLAGS_NONE, &metadata, scratchImg);
	}
	void CreateMipMap()
	{
		HRESULT result = GenerateMipMaps(scratchImg.GetImages(), scratchImg.GetImageCount(),
			scratchImg.GetMetadata(), TEX_FILTER_DEFAULT, 0, mipChain);
		if (SUCCEEDED(result))
		{
			scratchImg = std::move(mipChain);
			metadata = scratchImg.GetMetadata();
		}
	}
	void Transfer();
	void CreateView();
};