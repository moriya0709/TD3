#include "PostEffect.h"
#include "DirectXCommon.h"
#include "WindowAPI.h"
#include "SrvManager.h"
#include "Camera.h"

std::unique_ptr <PostEffect> PostEffect::instance = nullptr;

void PostEffect::Initialize(DirectXCommon* dxCommon, WindowAPI* windowAPI, SrvManager* srvManager) {
	// 引数で受け取ってメンバ変数に記録する
	dxCommon_ = dxCommon;
	windowAPI_ = windowAPI;
	srvManager_ = srvManager;

	// ルートシグネイチャ生成
	CreateRootSignature();
	// グラフィックスパイプライン生成
	CreateGraphicsPipeline();

	// ビューポートの初期化
	InitializeViewport();
	// シザリング矩形の初期化
	InitializeScissorRect();

	// ==========================================
	// ★修正1： Allocate(3) を Allocate(1) に変更！
	// ==========================================
	srvIndex_ = srvManager_->Allocate(1);               // メイン画像(t0)用
	uint32_t emptySrvIndex = srvManager_->Allocate(1);  // 空っぽの画像(t1)用
	depthSrvIndex_ = srvManager_->Allocate(1);          // 深度バッファ(t2)用 ★メンバ変数に保存！

	// レンダーターゲット
	renderTarget_ =
		CreateRenderTarget(
			dxCommon_->GetDevice(),
			windowAPI_->kClientWidth,
			windowAPI_->kClientHeight,
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			clearColor,
			dxCommon_->GetRtvHeap(),
			3,
			dxCommon_->GetSrvHeap(),
			srvIndex_
		);

	// --- ここから変更：ブルーム用の縮小レンダーターゲット作成（マルチパス化） ---
	// RTVインデックスは既存と被らない値（4からスタート）
	uint32_t currentRtvIndex = 4;

	for (int i = 0; i < kBloomPassCount; ++i) {
		// i=0 のとき 1/2, i=1 のとき 1/4, i=2 のとき 1/8 になる
		float divisor = powf(2.0f, (float)(i + 1));
		uint32_t width = (uint32_t)(windowAPI_->kClientWidth / divisor);
		uint32_t height = (uint32_t)(windowAPI_->kClientHeight / divisor);

		// ① 高輝度抽出用
		bloomBuffers_[i].lumSrvIndex = srvManager_->Allocate(1);
		bloomBuffers_[i].lumRenderTarget = CreateRenderTarget(
			dxCommon_->GetDevice(), width, height,
			DXGI_FORMAT_R16G16B16A16_FLOAT, clearColor,
			dxCommon_->GetRtvHeap(), currentRtvIndex++, dxCommon_->GetSrvHeap(), bloomBuffers_[i].lumSrvIndex
		);

		// ② ぼかし用 1 (X方向)
		bloomBuffers_[i].blurSrvIndex[0] = srvManager_->Allocate(1);
		bloomBuffers_[i].blurRenderTarget[0] = CreateRenderTarget(
			dxCommon_->GetDevice(), width, height,
			DXGI_FORMAT_R16G16B16A16_FLOAT, clearColor,
			dxCommon_->GetRtvHeap(), currentRtvIndex++, dxCommon_->GetSrvHeap(), bloomBuffers_[i].blurSrvIndex[0]
		);

		// ③ ぼかし用 2 (Y方向)
		bloomBuffers_[i].blurSrvIndex[1] = srvManager_->Allocate(1);
		bloomBuffers_[i].blurRenderTarget[1] = CreateRenderTarget(
			dxCommon_->GetDevice(), width, height,
			DXGI_FORMAT_R16G16B16A16_FLOAT, clearColor,
			dxCommon_->GetRtvHeap(), currentRtvIndex++, dxCommon_->GetSrvHeap(), bloomBuffers_[i].blurSrvIndex[1]
		);
	}

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
	// ブルーム
	effectData->bloomThreshold = 1.0f;
	effectData->bloomIntensity = 1.0f;
	effectData->bloomBlurRadius = 1.0f;

	effectData->intensity = 1.0f; // エフェクトの強さ
}

void PostEffect::Draw() {
	auto cmdList = dxCommon_->GetCommandList();

	// 深度バッファをシェーダーで読める状態にする
	TransitionDepthBuffer(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// --- 共通設定 ---
	cmdList->SetPipelineState(graphicsPipelineState.Get());
	cmdList->SetGraphicsRootSignature(rootSignature.Get());
	ID3D12DescriptorHeap* heaps[] = { dxCommon_->GetSrvHeap() };
	cmdList->SetDescriptorHeaps(1, heaps);

	// ==========================================
	// パス1：高輝度抽出 (1/2サイズに対して1回だけ行う)
	// ==========================================
	uint32_t passId = 1;
	// ★ ルートシグネチャの拡張に伴い、パス番号のインデックスは「6」になります
	cmdList->SetGraphicsRoot32BitConstants(6, 1, &passId, 0);

	float halfWidth = (float)windowAPI_->kClientWidth / 2.0f;
	float halfHeight = (float)windowAPI_->kClientHeight / 2.0f;
	D3D12_VIEWPORT halfViewport = { 0.0f, 0.0f, halfWidth, halfHeight, 0.0f, 1.0f };
	D3D12_RECT halfScissor = { 0, 0, (long)halfWidth, (long)halfHeight };

	TransitionResource(bloomBuffers_[0].lumRenderTarget.resource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	cmdList->OMSetRenderTargets(1, &bloomBuffers_[0].lumRenderTarget.rtvHandle, FALSE, nullptr);
	cmdList->ClearRenderTargetView(bloomBuffers_[0].lumRenderTarget.rtvHandle, clearColor, 0, nullptr);
	cmdList->RSSetViewports(1, &halfViewport);
	cmdList->RSSetScissorRects(1, &halfScissor);

	cmdList->SetGraphicsRootDescriptorTable(0, renderTarget_.srvGpuHandle); // t0: 元の絵
	cmdList->SetGraphicsRootConstantBufferView(1, effectResource->GetGPUVirtualAddress()); // b0: 定数バッファ
	cmdList->SetGraphicsRootDescriptorTable(2, renderTarget_.srvGpuHandle); // t1: ダミー
	cmdList->SetGraphicsRootDescriptorTable(3, renderTarget_.srvGpuHandle); // t2: ダミー
	cmdList->SetGraphicsRootDescriptorTable(4, renderTarget_.srvGpuHandle); // t3: ダミー
	cmdList->SetGraphicsRootDescriptorTable(5, dxCommon_->GetSRVGPUDescriptorHandle(depthSrvIndex_)); // t4: 深度

	cmdList->DrawInstanced(3, 1, 0, 0);

	TransitionResource(bloomBuffers_[0].lumRenderTarget.resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// ==========================================
	// パス2＆3：各サイズごとにぼかしを連鎖させる（3回ループ）
	// ==========================================
	for (int i = 0; i < 3; ++i) { // ★ 3段階（1/2, 1/4, 1/8）のブルームを作るループ
		float divisor = powf(2.0f, (float)(i + 1));
		float currentWidth = (float)windowAPI_->kClientWidth / divisor;
		float currentHeight = (float)windowAPI_->kClientHeight / divisor;
		D3D12_VIEWPORT currentViewport = { 0.0f, 0.0f, currentWidth, currentHeight, 0.0f, 1.0f };
		D3D12_RECT currentScissor = { 0, 0, (long)currentWidth, (long)currentHeight };
		cmdList->RSSetViewports(1, &currentViewport);
		cmdList->RSSetScissorRects(1, &currentScissor);

		// ★ ぼかしを2回繰り返す（往復させる）
		for (int iter = 0; iter < 2; ++iter) {
			// --- パス2：X方向ぼかし ---
			passId = 2;
			cmdList->SetGraphicsRoot32BitConstants(6, 1, &passId, 0);

			TransitionResource(bloomBuffers_[i].blurRenderTarget[0].resource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
			cmdList->OMSetRenderTargets(1, &bloomBuffers_[i].blurRenderTarget[0].rtvHandle, FALSE, nullptr);

			// 入力：1回目は前のバッファから、2回目以降は自分のYぼかし結果から
			D3D12_GPU_DESCRIPTOR_HANDLE inputSrvX;
			if (iter == 0) {
				inputSrvX = (i == 0) ? bloomBuffers_[0].lumRenderTarget.srvGpuHandle : bloomBuffers_[i - 1].blurRenderTarget[1].srvGpuHandle;
			} else {
				inputSrvX = bloomBuffers_[i].blurRenderTarget[1].srvGpuHandle;
			}

			cmdList->SetGraphicsRootDescriptorTable(0, inputSrvX); // t0: 入力画像
			cmdList->SetGraphicsRootDescriptorTable(2, inputSrvX); // t1: ダミー
			cmdList->SetGraphicsRootDescriptorTable(3, inputSrvX); // t2: ダミー
			cmdList->SetGraphicsRootDescriptorTable(4, inputSrvX); // t3: ダミー
			cmdList->SetGraphicsRootDescriptorTable(5, dxCommon_->GetSRVGPUDescriptorHandle(depthSrvIndex_)); // t4: 深度
			cmdList->DrawInstanced(3, 1, 0, 0);
			TransitionResource(bloomBuffers_[i].blurRenderTarget[0].resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			// --- パス3：Y方向ぼかし ---
			passId = 3;
			cmdList->SetGraphicsRoot32BitConstants(6, 1, &passId, 0);
			TransitionResource(bloomBuffers_[i].blurRenderTarget[1].resource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
			cmdList->OMSetRenderTargets(1, &bloomBuffers_[i].blurRenderTarget[1].rtvHandle, FALSE, nullptr);

			cmdList->SetGraphicsRootDescriptorTable(0, bloomBuffers_[i].blurRenderTarget[0].srvGpuHandle); // t0: Xぼかし結果
			cmdList->SetGraphicsRootDescriptorTable(2, bloomBuffers_[i].blurRenderTarget[0].srvGpuHandle); // t1: ダミー
			cmdList->SetGraphicsRootDescriptorTable(3, bloomBuffers_[i].blurRenderTarget[0].srvGpuHandle); // t2: ダミー
			cmdList->SetGraphicsRootDescriptorTable(4, bloomBuffers_[i].blurRenderTarget[0].srvGpuHandle); // t3: ダミー
			cmdList->SetGraphicsRootDescriptorTable(5, dxCommon_->GetSRVGPUDescriptorHandle(depthSrvIndex_)); // t4: 深度
			cmdList->DrawInstanced(3, 1, 0, 0);

			TransitionResource(bloomBuffers_[i].blurRenderTarget[1].resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		}
	}

	// ==========================================
	// パス4：最終合成 (バックバッファへ描画)
	// ==========================================
	passId = 0;
	cmdList->SetGraphicsRoot32BitConstants(6, 1, &passId, 0);
	cmdList->SetPipelineState(graphicsPipelineStateFinal.Get()); // UNORM_SRGB対応のPSO

	cmdList->RSSetViewports(1, &viewport_);
	cmdList->RSSetScissorRects(1, &scissorRect_);

	TransitionBackBuffer(D3D12_RESOURCE_STATE_RENDER_TARGET);
	D3D12_CPU_DESCRIPTOR_HANDLE backBufferRtv = dxCommon_->GetBackBufferRTVHandle();
	cmdList->OMSetRenderTargets(1, &backBufferRtv, FALSE, nullptr);

	// ★ ここが超重要！完成した3枚のぼかし画像をすべてシェーダーに渡す！
	cmdList->SetGraphicsRootDescriptorTable(0, renderTarget_.srvGpuHandle); // t0: メイン画像
	cmdList->SetGraphicsRootDescriptorTable(2, bloomBuffers_[0].blurRenderTarget[1].srvGpuHandle); // t1: 1/2ぼかし
	cmdList->SetGraphicsRootDescriptorTable(3, bloomBuffers_[1].blurRenderTarget[1].srvGpuHandle); // t2: 1/4ぼかし
	cmdList->SetGraphicsRootDescriptorTable(4, bloomBuffers_[2].blurRenderTarget[1].srvGpuHandle); // t3: 1/8ぼかし
	cmdList->SetGraphicsRootDescriptorTable(5, dxCommon_->GetSRVGPUDescriptorHandle(depthSrvIndex_)); // t4: 深度

	cmdList->DrawInstanced(3, 1, 0, 0);

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

	SetInverseViewProjectionMatrix(finalMat);

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

void PostEffect::TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
	// 変更前と変更後が同じなら何もしない
	if (before == after) return;

	// バリア（状態遷移の指示書）を作成
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = resource;
	barrier.Transition.StateBefore = before; // 現在の状態
	barrier.Transition.StateAfter = after;   // これから変えたい状態
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	// コマンドリストに指示を積む
	dxCommon_->GetCommandList()->ResourceBarrier(1, &barrier);
}

void PostEffect::CreateRootSignature() {
	// 各テクスチャ用の DescriptorRange
	D3D12_DESCRIPTOR_RANGE range0[1] = {}; range0[0].BaseShaderRegister = 0; range0[0].NumDescriptors = 1; range0[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; range0[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	D3D12_DESCRIPTOR_RANGE range1[1] = {}; range1[0].BaseShaderRegister = 1; range1[0].NumDescriptors = 1; range1[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; range1[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	D3D12_DESCRIPTOR_RANGE range2[1] = {}; range2[0].BaseShaderRegister = 2; range2[0].NumDescriptors = 1; range2[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; range2[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	D3D12_DESCRIPTOR_RANGE range3[1] = {}; range3[0].BaseShaderRegister = 3; range3[0].NumDescriptors = 1; range3[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; range3[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	D3D12_DESCRIPTOR_RANGE range4[1] = {}; range4[0].BaseShaderRegister = 4; range4[0].NumDescriptors = 1; range4[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; range4[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// RootParameter を 7つ に拡張
	D3D12_ROOT_PARAMETER rootParameters[7] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; rootParameters[0].DescriptorTable.pDescriptorRanges = range0; rootParameters[0].DescriptorTable.NumDescriptorRanges = 1; // t0: メイン画像
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; rootParameters[1].Descriptor.ShaderRegister = 0; // b0: 定数バッファ
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; rootParameters[2].DescriptorTable.pDescriptorRanges = range1; rootParameters[2].DescriptorTable.NumDescriptorRanges = 1; // t1: 1/2ブルーム
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; rootParameters[3].DescriptorTable.pDescriptorRanges = range2; rootParameters[3].DescriptorTable.NumDescriptorRanges = 1; // t2: 1/4ブルーム
	rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; rootParameters[4].DescriptorTable.pDescriptorRanges = range3; rootParameters[4].DescriptorTable.NumDescriptorRanges = 1; // t3: 1/8ブルーム
	rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; rootParameters[5].DescriptorTable.pDescriptorRanges = range4; rootParameters[5].DescriptorTable.NumDescriptorRanges = 1; // t4: 深度

	// b1用 (パス番号)
	rootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS; rootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; rootParameters[6].Constants.ShaderRegister = 1; rootParameters[6].Constants.Num32BitValues = 1; rootParameters[6].Constants.RegisterSpace = 0;

	// Sampler設定
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	assert(SUCCEEDED(hr));
	hr = dxCommon_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
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

	// ★修正1：深度テストを完全に無効化する
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_NEVER;
	psoDesc.DepthStencilState.StencilEnable = FALSE;

	// ★修正2：DSVフォーマットを「使わない（UNKNOWN）」に設定する
	// これが D24_... などになっていると、Draw時にDSVをセットしないと怒られます
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	psoDesc.SampleDesc.Count = 1;

	// ① パス1?3用（FLOAT）のPSO作成
	HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(
		&psoDesc, IID_PPV_ARGS(&graphicsPipelineState)
	);
	assert(SUCCEEDED(hr));

	// ==========================================
	// ★ 追加：パス4（バックバッファ最終合成）用のPSO作成
	// ==========================================
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // バックバッファのフォーマットに戻す！

	hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(
		&psoDesc, IID_PPV_ARGS(&graphicsPipelineStateFinal) // ★ 新しい変数に入れる
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
