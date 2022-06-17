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
	D3D12_HEAP_PROPERTIES heapProp;

	void SetResource(size_t width, size_t height, D3D12_RESOURCE_DIMENSION Dimension);
	void SetHeapProp(D3D12_HEAP_TYPE Type, D3D12_CPU_PAGE_PROPERTY CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN)
	{
		heapProp = {};
		heapProp.Type = Type;
		heapProp.CPUPageProperty = CPUPageProperty;
		heapProp.MemoryPoolPreference = MemoryPoolPreference;
	}
	void CreateBuffer(ID3D12Device* device);
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
	void SetResource();
	void CreateMipMap();
	void Transfer();
	void CreateView();
};