#include "MyClass.h"

ShaderBlob::ShaderBlob(const LPCWSTR fileName, const LPCSTR target, ID3DBlob* errorBlob)
{
	HRESULT result;

	result = D3DCompileFromFile(
		fileName, // �V�F�[�_�t�@�C����
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // �C���N���[�h�\�ɂ���
		"main", target, // �G���g���[�|�C���g���A�V�F�[�_�[���f���w��
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // �f�o�b�O�p�ݒ�
		0,
		&blob, &errorBlob);

	if (FAILED(result)) {
		// errorBlob����G���[���e��string�^�ɃR�s�[
		std::string error;
		error.resize(errorBlob->GetBufferSize());
		std::copy_n((char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			error.begin());
		error += "\n";
		// �G���[���e���o�̓E�B���h�E�ɕ\��
		OutputDebugStringA(error.c_str());
		assert(0);
	}
}

// �E�B���h�E�v���V�[�W��
LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// ���b�Z�[�W�ɉ����ăQ�[���ŗL�̏������s��
	switch (msg)
	{
		// �E�B���h�E���j�����ꂽ
	case WM_DESTROY:
		// OS�ɑ΂��āA�A�v���̏I����`����
		PostQuitMessage(0);
		return 0;
	}

	// �W���̃��b�Z�[�W�������s��
	return DefWindowProc(hwnd, msg, wparam, lparam);
}
WindowsAPI::WindowsAPI(WNDPROC lpfnWndProc, Int2 WIN_SIZE)
{
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = lpfnWndProc; // �E�B���h�E�v���V�[�W����ݒ�
	w.lpszClassName = L"DirectXGame"; // �E�B���h�E�N���X��
	w.hInstance = GetModuleHandle(nullptr); // �E�B���h�E�n���h��
	w.hCursor = LoadCursor(NULL, IDC_ARROW); // �J�[�\���w��

	// �E�B���h�E�N���X��OS�ɓo�^����
	RegisterClassEx(&w);

	wrc = { 0, 0, WIN_SIZE.width, WIN_SIZE.height };
	// �����ŃT�C�Y��␳����
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	hwnd = CreateWindow(w.lpszClassName, // �N���X��
		L"LE2A_16_�i�J���}_�V���E��_CG2", // �^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW, // �W���I�ȃE�B���h�E�X�^�C��
		CW_USEDEFAULT, // �\��X���W(OS�ɔC����)
		CW_USEDEFAULT, // �\��Y���W(OS�ɔC����)
		wrc.right - wrc.left, // �E�B���h�E����
		wrc.bottom - wrc.top, // �E�B���h�E�c��
		nullptr, // �e�E�B���h�E�n���h��
		nullptr, // ���j���[�n���h��
		w.hInstance, // �Ăяo���A�v���P�[�V�����n���h��
		nullptr); // �I�v�V����
}
void WindowsAPI::MyUnregisterClass()
{
	UnregisterClass(w.lpszClassName, w.hInstance);
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
	desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; // �W���ݒ�
}
void Pipeline::SetRasterizer()
{
	desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // �J�����O���Ȃ�
	desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; // �|���S�����h��Ԃ�
	desc.RasterizerState.DepthClipEnable = true; // �[�x�N���b�s���O��L����
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
	desc.NumRenderTargets = 1; // �`��Ώۂ�1��
	desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 0~255�w���RGBA
	desc.SampleDesc.Count = 1; // 1�s�N�Z���ɂ�1��T���v�����O
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
		params[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// �萔�o�b�t�@�r���[
		params[i].Descriptor.ShaderRegister = j++;					// �萔�o�b�t�@�ԍ�
		params[i].Descriptor.RegisterSpace = 0;						// �f�t�H���g�l
		params[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	// �S�ẴV�F�[�_���猩����
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
	// �p�t�H�[�}���X���������̂��珇�ɁA�S�ẴA�_�v�^�[��񋓂���
	for (UINT i = 0;
		dxgiFactory->EnumAdapterByGpuPreference(i,
			DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
			IID_PPV_ARGS(&tmpAdapter)) != DXGI_ERROR_NOT_FOUND;
		i++)
	{
		// ���I�z��ɒǉ�����
		adapters.push_back(tmpAdapter);
	}
	// �Ó��ȃA�_�v�^��I�ʂ���
	for (size_t i = 0; i < adapters.size(); i++)
	{
		DXGI_ADAPTER_DESC3 adapterDesc;
		// �A�_�v�^�[�̏����擾����
		adapters[i]->GetDesc3(&adapterDesc);
		// �\�t�g�E�F�A�f�o�C�X�����
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
		{
			// �f�o�C�X���̗p���ă��[�v�𔲂���
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
		// �̗p�����A�_�v�^�[�Ńf�o�C�X�𐶐�
		result = D3D12CreateDevice(tmpAdapter, levels[i], IID_PPV_ARGS(&device));
		if (result == S_OK)
		{
			// �f�o�C�X�𐶐��ł������_�Ń��[�v�𔲂���
			featureLevel = levels[i];
			return device;
		}
	}
}

SwapChain::SwapChain()
{
	desc.Width = 1280;
	desc.Height = 720;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // �F���̏���
	desc.SampleDesc.Count = 1; // �}���`�T���v�����Ȃ�
	desc.BufferUsage = DXGI_USAGE_BACK_BUFFER; // �o�b�N�o�b�t�@�p
	desc.BufferCount = 2; // �o�b�t�@����2�ɐݒ�
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // �t���b�v��͔j��
	desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	backBuffers.resize(desc.BufferCount);
	rtvDesc = {};
	sc = nullptr;
}
void SwapChain::Create(IDXGIFactory7* dxgiFactory, ID3D12CommandQueue* commandQueue, HWND hwnd)
{
	assert(SUCCEEDED(
		dxgiFactory->CreateSwapChainForHwnd(
			commandQueue, hwnd, &desc, nullptr, nullptr,
			(IDXGISwapChain1**)&sc)));
}
void SwapChain::Set(ID3D12Device* device, ID3D12DescriptorHeap* rtvHeap, D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc)
{
	for (size_t i = 0; i < backBuffers.size(); i++) {
		sc->GetBuffer((UINT)i, IID_PPV_ARGS(&backBuffers[i]));
		rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += i * device->GetDescriptorHandleIncrementSize(rtvHeapDesc.Type);
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		device->CreateRenderTargetView(backBuffers[i], &rtvDesc, rtvHandle);
	}
}

void UseBlendMode(D3D12_RENDER_TARGET_BLEND_DESC& blenddesc)
{
	blenddesc.BlendEnable = true;
	blenddesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;	// ���Z
	blenddesc.SrcBlendAlpha = D3D12_BLEND_ONE;		// �\�[�X�̒l��100%�g��
	blenddesc.DestBlendAlpha = D3D12_BLEND_ZERO;	// �g��Ȃ�
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