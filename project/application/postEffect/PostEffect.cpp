#include "PostEffect.h"
#include "DirectXCommon.h"
#include "WindowAPI.h"
#include "Camera.h"

std::unique_ptr <PostEffect> PostEffect::instance = nullptr;

void PostEffect::Initialize(DirectXCommon* dxCommon, WindowAPI* windowAPI) {
	// 引数で受け取ってメンバ変数に記録する
	dxCommon_ = dxCommon;
	windowAPI_ = windowAPI;

	// ルートシグネイチャ生成
	CreateRootSignature();
	// グラフィックスパイプライン生成
	CreateGraphicsPipeline();

	// ビューポートの初期化
	InitializeViewport();
	// シザリング矩形の初期化
	InitializeScissorRect();

	// レンダーターゲット
	renderTarget_ =
		CreateRenderTarget(
			dxCommon_->GetDevice(),
			windowAPI_->kClientWidth,
			windowAPI_->kClientHeight,
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			clearColor,
			dxCommon_->GetRtvHeap(),
			3,
			dxCommon_->GetSrvHeap(),
			10
		);

	currentState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	// エフェクト
	effectResource = dxCommon_->CreateBufferResource(sizeof(EffectData));
	effectResource->Map(0, nullptr, reinterpret_cast<void**>(&effectData));
	effectData->isInversion = false;
	effectData->isGrayscale = false;
	effectData->isRadialBlur = false;
	effectData->isDistanceFog = false;
	effectData->isHeightFog = false;
	effectData->isDOF = true;
	// 放射線ブラー用のパラメータ
	effectData->blurCenter = { 0.5f,0.5f };
	effectData->blurWidth = 0.01f;
	effectData->blurSamples = 10;
	// ディスタンスフォグ用のパラメータ
	effectData->distanceFogColor = { 0.5f,0.5f,0.5f };// フォグの色
	effectData->distanceFogStart = 0.0f;  // フォグが始まる距離
	effectData->distanceFogEnd = 10.0f; // 完全にフォグに覆われる距離
	effectData->zNear = 0.1f; // カメラのニアクリップ面
	effectData->zFar = 1000.0f; // カメラのファークリップ面
	// ハイトフォグ用のパラメータ
	effectData->heightFogColor = { 0.5f,0.5f,0.5f }; // フォグの色
	effectData->heightFogTop = 10.0f; // フォグが始まる高さ
	effectData->heightFogBottom = 0.0f; // 完全にフォグに覆われる高さ
	effectData->heightFogDensity = 1.0f;
	// DOF用のパラメータ
	effectData->focusDistance = 5.0f; // ピントが合う距離
	effectData->focusRange = 2.0f; // ピントが合う範囲（遊び）
	effectData->bokehRadius = 5.0f; // ボケの最大半径

	effectData->intensity = 1.0f; // エフェクトの強さ
}

void PostEffect::Draw() {
	// --- 準備 ---
	// 1. 深度バッファを「読み取り用(SRV)」に変える
	TransitionDepthBuffer(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// 2. バックバッファを「書き込み用(RTV)」に変える
	TransitionBackBuffer(D3D12_RESOURCE_STATE_RENDER_TARGET);

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dxCommon_->GetDSVHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE backBufferRtv = dxCommon_->GetBackBufferRTVHandle();
	dxCommon_->GetCommandList()->OMSetRenderTargets(1, &backBufferRtv, FALSE, &dsvHandle);
	dxCommon_->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());
	dxCommon_->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());
	ID3D12DescriptorHeap* heaps[] = { dxCommon_->srvDescriptorHeap.Get() };
	dxCommon_->GetCommandList()->SetDescriptorHeaps(1, heaps);
	dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(0, dxCommon_->GetSRVGPUDescriptorHandle(10));

	// エフェクト切り替え
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, effectResource->GetGPUVirtualAddress());

	// 全画面トライアングルを描画
	dxCommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0);

	// --- 後始末 (次のフレームのために) ---
	// 3. 深度バッファを「書き込み用(DEPTH_WRITE)」に戻す
	TransitionDepthBuffer(D3D12_RESOURCE_STATE_DEPTH_WRITE);

}

// 描画前処理
void PostEffect::PreDraw() {
	// バリア
	Transition(D3D12_RESOURCE_STATE_RENDER_TARGET);

	// RTVセット
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dxCommon_->GetDSVHandle();
	dxCommon_->GetCommandList()->OMSetRenderTargets(1,&renderTarget_.rtvHandle,FALSE, &dsvHandle);

	// クリア
	dxCommon_->GetCommandList()->ClearRenderTargetView(renderTarget_.rtvHandle, clearColor, 0, nullptr);

	// Viewport
	dxCommon_->GetCommandList()->RSSetViewports(1, &viewport_);
	dxCommon_->GetCommandList()->RSSetScissorRects(1, &scissorRect_);
}

// 描画後処理
void PostEffect::PostDraw() {
	Transition(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void PostEffect::HightFogUpdate(Camera* camera) {
	Matrix4x4 camWorldMat = camera->GetWorldMatrix();

	// プロジェクションの逆行列を作る
	Matrix4x4 projMat = camera->GetProjectionMatrix();
	DirectX::XMMATRIX mProj = DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(&projMat));
	DirectX::XMVECTOR det;
	DirectX::XMMATRIX invProj = DirectX::XMMatrixInverse(&det, mProj);

	// プロジェクション逆行列 × カメラのワールド行列の合成
	DirectX::XMMATRIX mCamWorld = DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(&camWorldMat));
	DirectX::XMMATRIX resultInvVP = invProj * mCamWorld;

	// 転置してセット
	resultInvVP = DirectX::XMMatrixTranspose(resultInvVP);
	Matrix4x4 finalMat;
	DirectX::XMStoreFloat4x4(reinterpret_cast<DirectX::XMFLOAT4X4*>(&finalMat), resultInvVP);

	PostEffect::GetInstance()->SetInverseViewProjectionMatrix(finalMat);

}

PostEffect* PostEffect::GetInstance() {
	if (instance == nullptr) {
		instance = std::make_unique <PostEffect>();
	}
	return instance.get();
}


// レンダーターゲットの生成
RenderTarget PostEffect::CreateRenderTarget(
	ID3D12Device* device,
	uint32_t width,
	uint32_t height,
	DXGI_FORMAT format,
	const float clearColor[4],
	ID3D12DescriptorHeap* rtvHeap,
	UINT rtvIndex,
	ID3D12DescriptorHeap* srvHeap,
	UINT srvIndex
) {
	RenderTarget rt;

	// *リソース作成* //
	D3D12_HEAP_PROPERTIES heapProp{};
	heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = width;
	desc.Height = height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = format;
	memcpy(clearValue.Color, clearColor, sizeof(float) * 4);

	HRESULT hr = device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(&rt.resource)
	);
	assert(SUCCEEDED(hr));

	// *RTV作成* //
	UINT rtvDescriptorSize =
		device->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	rt.rtvHandle =
		rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rt.rtvHandle.ptr += rtvIndex * rtvDescriptorSize;

	device->CreateRenderTargetView(
		rt.resource.Get(),
		nullptr,
		rt.rtvHandle
	);

	// *SRV作成* //
	UINT srvDescriptorSize =
		device->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	rt.srvCpuHandle =
		srvHeap->GetCPUDescriptorHandleForHeapStart();
	rt.srvCpuHandle.ptr += srvIndex * srvDescriptorSize;

	rt.srvGpuHandle =
		srvHeap->GetGPUDescriptorHandleForHeapStart();
	rt.srvGpuHandle.ptr += srvIndex * srvDescriptorSize;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = format;
	srvDesc.Shader4ComponentMapping =
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	device->CreateShaderResourceView(
		rt.resource.Get(),
		&srvDesc,
		rt.srvCpuHandle
	);

	return rt;
}

// リソースバリアの発行
void PostEffect::Transition(D3D12_RESOURCE_STATES newState) {
	if (currentState_ == newState) return;

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = renderTarget_.resource.Get();
	barrier.Transition.StateBefore = currentState_;
	barrier.Transition.StateAfter = newState;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	dxCommon_->GetCommandList()->ResourceBarrier(1, &barrier);

	currentState_ = newState;
}

void PostEffect::TransitionBackBuffer(D3D12_RESOURCE_STATES newState) {
	UINT idx = dxCommon_->swapChain->GetCurrentBackBufferIndex();

	if (dxCommon_->backBufferStates[idx] == newState) return;

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = dxCommon_->swapChainResources[idx].Get();
	barrier.Transition.StateBefore = dxCommon_->backBufferStates[idx];
	barrier.Transition.StateAfter = newState;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	dxCommon_->GetCommandList()->ResourceBarrier(1, &barrier);

	dxCommon_->backBufferStates[idx] = newState;
}

void PostEffect::TransitionDepthBuffer(D3D12_RESOURCE_STATES newState) {
	// 現在の深度バッファの状態を保持する変数（本来はクラスメンバーにするのが理想）
	static D3D12_RESOURCE_STATES currentDepthState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

	// 現在の状態と同じなら何もしない
	if (currentDepthState == newState) return;

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

	// 対象は「深度バッファのリソース」
	barrier.Transition.pResource = dxCommon_->GetDepthStencilResource();

	barrier.Transition.StateBefore = currentDepthState; // 前の状態
	barrier.Transition.StateAfter = newState;           // 次の状態（引数）
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	// GPUに命令を発行
	dxCommon_->GetCommandList()->ResourceBarrier(1, &barrier);

	// 状態を更新
	currentDepthState = newState;
}

void PostEffect::CreateRootSignature() {
	// DescriptorRange作成
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0; // 0から始まる
	descriptorRange[0].NumDescriptors = 3;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // offsetを自動計算

	// RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// RootParameter作成
	D3D12_ROOT_PARAMETER rootParameters[2] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRange; // Tableの中身の配列を指定
	rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // Tableで利用する数
	// エフェクト切り替え
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[1].Descriptor.ShaderRegister = 0; // b0

	descriptionRootSignature.pParameters = rootParameters; // ルートパラメーター配列へのポインタ
	descriptionRootSignature.NumParameters = _countof(rootParameters); // 配列の長さ

	// Samplerの設定
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // 倍リニアフィルター
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 0~1の範囲外をリピート
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // 比較しない
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX; // ありったけのMipmapを使う
	staticSamplers[0].ShaderRegister = 0; // レジスタ番号０を使う
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	// シリアライズ「してバイナリにする
	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		//Log(reinterpret_cast<char*> (errorBlob->GetBufferPointer()));
		assert(false);
	}
	// バイナリを元に生成
	hr = dxCommon_->GetDevice()->CreateRootSignature(0,
		signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));

}

void PostEffect::CreateGraphicsPipeline() {

	vertexShaderBlob = dxCommon_->CompileShader(
		L"Resource/shaders/PostEffect.VS.hlsl", L"vs_6_0");

	pixelShaderBlob = dxCommon_->CompileShader(
		L"Resource/shaders/PostEffect.PS.hlsl", L"ps_6_0");

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = { vertexShaderBlob->GetBufferPointer(),
				   vertexShaderBlob->GetBufferSize() };
	psoDesc.PS = { pixelShaderBlob->GetBufferPointer(),
				   pixelShaderBlob->GetBufferSize() };

	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	psoDesc.DepthStencilState = {};
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;

	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	psoDesc.SampleDesc.Count = 1;

	HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(
		&psoDesc,
		IID_PPV_ARGS(&graphicsPipelineState)
	);
	assert(SUCCEEDED(hr));
}

void PostEffect::InitializeViewport() {
	viewport_.Width = (float)windowAPI_->kClientWidth;
	viewport_.Height = (float)windowAPI_->kClientHeight;
	viewport_.TopLeftX = 0;
	viewport_.TopLeftY = 0;
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;
}

void PostEffect::InitializeScissorRect() {
	scissorRect_.left = 0;
	scissorRect_.top = 0;
	scissorRect_.right = windowAPI_->kClientWidth;
	scissorRect_.bottom = windowAPI_->kClientHeight;
}
