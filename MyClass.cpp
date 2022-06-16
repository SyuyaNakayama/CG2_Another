#include "MyClass.h"
void DirectInput::Initialize(WNDCLASSEX w)
{
	assert(SUCCEEDED(
		DirectInput8Create(w.hInstance, DIRECTINPUT_VERSION,
			IID_IDirectInput8, (void**)&input, nullptr)));
}

void Keyboard::GetInstance(WNDCLASSEX w)
{
	Initialize(w);
	assert(SUCCEEDED(input->CreateDevice(GUID_SysKeyboard, &device, NULL)));
}
void Keyboard::SetDataStdFormat()
{
	assert(SUCCEEDED(device->SetDataFormat(&c_dfDIKeyboard)));// 標準形式
}
void Keyboard::SetCooperativeLevel(HWND hwnd)
{
	assert(SUCCEEDED(device->SetCooperativeLevel(
		hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY)));
}
void Keyboard::GetDeviceState()
{
	device->GetDeviceState(sizeof(key), key);
}
void Keyboard::TransferOldkey()
{
	for (size_t i = 0; i < sizeof(oldkey); i++) { oldkey[i] = key[i]; }
}
bool Keyboard::isInput(const int KEY)
{
	if (key[KEY]) { return true; }
	return false;
}
bool Keyboard::isTrigger(const int KEY)
{
	return (!oldkey[KEY] && key[KEY]);
	return false;
}

ShaderBlob::ShaderBlob(const LPCWSTR fileName, const LPCSTR target, ID3DBlob* errorBlob)
{
	HRESULT result;

	result = D3DCompileFromFile(
		fileName, // シェーダファイル名
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルード可能にする
		"main", target, // エントリーポイント名、シェーダーモデル指定
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用設定
		0,
		&blob, &errorBlob);

	if (FAILED(result)) {
		// errorBlobからエラー内容をstring型にコピー
		std::string error;
		error.resize(errorBlob->GetBufferSize());
		std::copy_n((char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			error.begin());
		error += "\n";
		// エラー内容を出力ウィンドウに表示
		OutputDebugStringA(error.c_str());
		assert(0);
	}
}

// ウィンドウプロシージャ
LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// メッセージに応じてゲーム固有の処理を行う
	switch (msg)
	{
		// ウィンドウが破棄された
	case WM_DESTROY:
		// OSに対して、アプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}

	// 標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}
WindowsAPI::WindowsAPI(WNDPROC lpfnWndProc, Int2 WIN_SIZE)
{
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = lpfnWndProc; // ウィンドウプロシージャを設定
	w.lpszClassName = L"DirectXGame"; // ウィンドウクラス名
	w.hInstance = GetModuleHandle(nullptr); // ウィンドウハンドル
	w.hCursor = LoadCursor(NULL, IDC_ARROW); // カーソル指定

	// ウィンドウクラスをOSに登録する
	RegisterClassEx(&w);

	wrc = { 0, 0, WIN_SIZE.width, WIN_SIZE.height };
	// 自動でサイズを補正する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	hwnd = CreateWindow(w.lpszClassName, // クラス名
		L"LE2A_16_ナカヤマ_シュウヤ_CG2", // タイトルバーの文字
		WS_OVERLAPPEDWINDOW, // 標準的なウィンドウスタイル
		CW_USEDEFAULT, // 表示X座標(OSに任せる)
		CW_USEDEFAULT, // 表示Y座標(OSに任せる)
		wrc.right - wrc.left, // ウィンドウ横幅
		wrc.bottom - wrc.top, // ウィンドウ縦幅
		nullptr, // 親ウィンドウハンドル
		nullptr, // メニューハンドル
		w.hInstance, // 呼び出しアプリケーションハンドル
		nullptr); // オプション
}

void Buffer::SetResource(size_t width, size_t height, D3D12_RESOURCE_DIMENSION Dimension)
{
	resDesc.Dimension = Dimension;
	resDesc.Width = width;
	resDesc.Height = height;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
}
void Buffer::CreateBuffer(ID3D12Device* device, D3D12_HEAP_PROPERTIES heapProp)
{
	assert(SUCCEEDED(
		device->CreateCommittedResource(
			&heapProp, D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(&buff))));
}
void Buffer::Init()
{
	resDesc = {};
	buff = nullptr;
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

TextureBuf::TextureBuf()
{
	Init();
	view = {};
	metadata = {};
	scratchImg = {};
	mipChain = {};

	LoadFromWICFile(L"Resources/Map.png", WIC_FLAGS_NONE, &metadata, scratchImg);

	HRESULT result = GenerateMipMaps(scratchImg.GetImages(), scratchImg.GetImageCount(),
		scratchImg.GetMetadata(), TEX_FILTER_DEFAULT, 0, mipChain);
	if (SUCCEEDED(result))
	{
		scratchImg = std::move(mipChain);
		metadata = scratchImg.GetMetadata();
	}

	metadata.format = MakeSRGB(metadata.format);
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

Pipeline::Pipeline()
{
	desc = {};
	state = nullptr;
}
void Pipeline::SetShader(ShaderBlob vs, ShaderBlob ps)
{
	desc.VS.pShaderBytecode = vs.blob->GetBufferPointer();
	desc.VS.BytecodeLength = vs.blob->GetBufferSize();
	desc.PS.pShaderBytecode = ps.blob->GetBufferPointer();
	desc.PS.BytecodeLength = ps.blob->GetBufferSize();
}
void Pipeline::SetSampleMask()
{
	desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; // 標準設定
}
void Pipeline::SetRasterizer()
{
	desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // カリングしない
	desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; // ポリゴン内塗りつぶし
	desc.RasterizerState.DepthClipEnable = true; // 深度クリッピングを有効に
}
void Pipeline::SetInputLayout(D3D12_INPUT_ELEMENT_DESC* inputLayout, UINT layoutNum)
{
	desc.InputLayout.pInputElementDescs = inputLayout;
	desc.InputLayout.NumElements = layoutNum;
}
void Pipeline::SetPrimitiveTopology()
{
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
}
void Pipeline::SetOthers()
{
	desc.NumRenderTargets = 1; // 描画対象は1つ
	desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 0~255指定のRGBA
	desc.SampleDesc.Count = 1; // 1ピクセルにつき1回サンプリング
}
void Pipeline::CreatePipelineState(ID3D12Device* device)
{
	assert(SUCCEEDED(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&state))));
}

RootSignature::RootSignature()
{
	params[0] = {};
	params[1] = {};
	desc = {};
	rs = nullptr;
	blob = nullptr;
}
void RootSignature::SetParam(D3D12_DESCRIPTOR_RANGE descriptorRange)
{
	for (size_t i = 0, j = 0; i < 3; i += 2)
	{
		params[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// 定数バッファビュー
		params[i].Descriptor.ShaderRegister = j++;					// 定数バッファ番号
		params[i].Descriptor.RegisterSpace = 0;						// デフォルト値
		params[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	// 全てのシェーダから見える
	}

	params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	params[1].DescriptorTable.pDescriptorRanges = &descriptorRange;
	params[1].DescriptorTable.NumDescriptorRanges = 1;
	params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
}
void RootSignature::SetRootSignature(D3D12_STATIC_SAMPLER_DESC samplerDesc)
{
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	desc.pParameters = params;
	desc.NumParameters = _countof(params);
	desc.pStaticSamplers = &samplerDesc;
	desc.NumStaticSamplers = 1;
}
void RootSignature::SerializeRootSignature(ID3D12Device* device, ID3DBlob* errorBlob)
{
	HRESULT result = D3D12SerializeRootSignature(&desc,
		D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, &errorBlob);
	assert(SUCCEEDED(result));
	result = device->CreateRootSignature(0, blob->GetBufferPointer(),
		blob->GetBufferSize(), IID_PPV_ARGS(&rs));
	assert(SUCCEEDED(result));
	blob->Release();
}

DirectXInit::DirectXInit()
{
	assert(SUCCEEDED(CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory))));
}
void DirectXInit::AdapterChoice()
{
	// パフォーマンスが高いものから順に、全てのアダプターを列挙する
	for (UINT i = 0;
		dxgiFactory->EnumAdapterByGpuPreference(i,
			DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
			IID_PPV_ARGS(&tmpAdapter)) != DXGI_ERROR_NOT_FOUND;
		i++)
	{
		// 動的配列に追加する
		adapters.push_back(tmpAdapter);
	}
	// 妥当なアダプタを選別する
	for (size_t i = 0; i < adapters.size(); i++)
	{
		DXGI_ADAPTER_DESC3 adapterDesc;
		// アダプターの情報を取得する
		adapters[i]->GetDesc3(&adapterDesc);
		// ソフトウェアデバイスを回避
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
		{
			// デバイスを採用してループを抜ける
			tmpAdapter = adapters[i];
			break;
		}
	}
}
ID3D12Device* DirectXInit::CreateDevice(D3D_FEATURE_LEVEL* levels, size_t levelsNum, ID3D12Device* device)
{
	HRESULT result;

	for (size_t i = 0; i < levelsNum; i++)
	{
		// 採用したアダプターでデバイスを生成
		result = D3D12CreateDevice(tmpAdapter, levels[i], IID_PPV_ARGS(&device));
		if (result == S_OK)
		{
			// デバイスを生成できた時点でループを抜ける
			featureLevel = levels[i];
			return device;
		}
	}
}

void UseBlendMode(D3D12_RENDER_TARGET_BLEND_DESC& blenddesc)
{
	blenddesc.BlendEnable = true;
	blenddesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;	// 加算
	blenddesc.SrcBlendAlpha = D3D12_BLEND_ONE;		// ソースの値を100%使う
	blenddesc.DestBlendAlpha = D3D12_BLEND_ZERO;	// 使わない
}
void SetBlend(D3D12_RENDER_TARGET_BLEND_DESC& blenddesc, int blendMode)
{
	switch (blendMode)
	{
	case BLENDMODE_ADD:
		blenddesc.BlendOp = D3D12_BLEND_OP_ADD;
		blenddesc.SrcBlend = D3D12_BLEND_ONE;
		blenddesc.DestBlend = D3D12_BLEND_ONE;
		break;
	case BLENDMODE_SUB:
		blenddesc.BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
		blenddesc.SrcBlend = D3D12_BLEND_ONE;
		blenddesc.DestBlend = D3D12_BLEND_ONE;
		break;
	case BLENDMODE_COLORFLIP:
		blenddesc.BlendOp = D3D12_BLEND_OP_ADD;
		blenddesc.SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
		blenddesc.DestBlend = D3D12_BLEND_ZERO;
		break;
	case BLENDMODE_ALPHA:
		blenddesc.BlendOp = D3D12_BLEND_OP_ADD;
		blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blenddesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		break;
	}
}