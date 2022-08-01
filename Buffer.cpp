#include "Buffer.h"
void Buffer::Init()
{
	resDesc = {};
	buff = nullptr;
}
void Buffer::SetResource(size_t width, size_t height, D3D12_RESOURCE_DIMENSION Dimension)
{
	resDesc.Dimension = Dimension;
	resDesc.Width = (UINT)width;
	resDesc.Height = (UINT)height;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
}
void Buffer::SetHeapProp(D3D12_HEAP_TYPE Type, D3D12_CPU_PAGE_PROPERTY CPUPageProperty,
	D3D12_MEMORY_POOL MemoryPoolPreference)
{
	heapProp = {};
	heapProp.Type = Type;
	heapProp.CPUPageProperty = CPUPageProperty;
	heapProp.MemoryPoolPreference = MemoryPoolPreference;
}
void Buffer::CreateBuffer(ID3D12Device* device)
{
	device->CreateCommittedResource(
		&heapProp, D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&buff));
}

ConstBuf::ConstBuf(Type type)
{
	Init();
	this->type = type;
	switch (type)
	{
	case ConstBuf::Material:
		this->size = ((sizeof(ConstBufferDataMaterial) + 0xff) & ~0xff);
		break;
	case ConstBuf::Transform:
		this->size = ((sizeof(ConstBufferDataTransform) + 0xff) & ~0xff);
		break;
	default:
		this->size = 1;
		break;
	}
	mapMaterial = nullptr;
	mapTransform = nullptr;
}
void ConstBuf::Mapping()
{
	switch (type)
	{
	case ConstBuf::Material:
		assert(SUCCEEDED(buff->Map(0, nullptr, (void**)&mapMaterial)));
		break;
	case ConstBuf::Transform:
		assert(SUCCEEDED(buff->Map(0, nullptr, (void**)&mapTransform)));
		break;
	}
}

VertexBuf::VertexBuf(UINT size)
{
	Init();
	this->size = size;
	map = nullptr;
	view = {};
}
void VertexBuf::Mapping(Vertex* vertices, const int ARRAY_NUM)
{
	assert(SUCCEEDED(buff->Map(0, nullptr, (void**)&map)));

	for (int i = 0; i < ARRAY_NUM; i++) { map[i] = vertices[i]; }
	buff->Unmap(0, nullptr);
}
void VertexBuf::CreateView()
{
	view.BufferLocation = buff->GetGPUVirtualAddress();
	view.SizeInBytes = size;
	view.StrideInBytes = sizeof(Vertex);
}

IndexBuf::IndexBuf(UINT size)
{
	Init();
	this->size = size;
	map = nullptr;
	view = {};
}
void IndexBuf::Mapping(uint16_t* indices, const int ARRAY_NUM)
{
	assert(SUCCEEDED(buff->Map(0, nullptr, (void**)&map)));

	for (int i = 0; i < ARRAY_NUM; i++) { map[i] = indices[i]; }
	buff->Unmap(0, nullptr);
}
void IndexBuf::CreateView()
{
	view.BufferLocation = buff->GetGPUVirtualAddress();
	view.Format = DXGI_FORMAT_R16_UINT;
	view.SizeInBytes = size;
}

TextureBuf::TextureBuf(const wchar_t* texName)
{
	Init();
	view = {};
	metadata = {};
	scratchImg = {};
	mipChain = {};

	LoadFromWICFile(texName, WIC_FLAGS_NONE, &metadata, scratchImg);

	HRESULT result = GenerateMipMaps(scratchImg.GetImages(), scratchImg.GetImageCount(),
		scratchImg.GetMetadata(), TEX_FILTER_DEFAULT, 0, mipChain);
	if (SUCCEEDED(result))
	{
		scratchImg = std::move(mipChain);
		metadata = scratchImg.GetMetadata();
	}

	metadata.format = MakeSRGB(metadata.format);
}
void TextureBuf::SetResource()
{
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Format = metadata.format;
	resDesc.Width = metadata.width;
	resDesc.Height = (UINT)metadata.height;
	resDesc.DepthOrArraySize = (UINT16)metadata.arraySize;
	resDesc.MipLevels = (UINT16)metadata.mipLevels;
	resDesc.SampleDesc.Count = 1;
}
void TextureBuf::CreateMipMap()
{
	HRESULT result = GenerateMipMaps(scratchImg.GetImages(), scratchImg.GetImageCount(),
		scratchImg.GetMetadata(), TEX_FILTER_DEFAULT, 0, mipChain);
	if (SUCCEEDED(result))
	{
		scratchImg = std::move(mipChain);
		metadata = scratchImg.GetMetadata();
	}
}
void TextureBuf::Transfer()
{
	HRESULT result;

	for (size_t i = 0; i < metadata.mipLevels; i++)
	{
		const Image* IMG = scratchImg.GetImage(i, 0, 0);
		result = buff->WriteToSubresource(
			(UINT)i, nullptr, IMG->pixels,
			(UINT)IMG->rowPitch, (UINT)IMG->slicePitch);
		assert(SUCCEEDED(result));
	}
}
void TextureBuf::CreateView()
{
	view.Format = resDesc.Format;
	view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	view.Texture2D.MipLevels = resDesc.MipLevels;
}