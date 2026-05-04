#include "BookUi.h"
#include "DirectXCommon.h"
#include "TextureManager.h"
#include "BookUiCommon.h"
#include "WindowAPI.h"
#include "SpriteCommon.h"

void BookUi::Initialize(std::string textureFilePath) {
	dxCommon_ = DirectXCommon::GetInstance();
	textureFilePath_ = textureFilePath;
	windowAPI_ = BookUiCommon::GetInstance()->GetWindowAPI();

	// *頂点データ* //

	// 頂点データは4つでOK
	vertexResource = dxCommon_->CreateBufferResource(sizeof(VertexData) * 4);

	// ★ ここに追加：GPUアドレスとストライド（1頂点分のサイズ）の設定が抜けていました！
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = sizeof(VertexData) * 4;
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	// データを書き込む (左上→右上→右下→左下の順)
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	// 0: 左上
	vertexData[0].position = { 0.0f, 0.0f, 0.0f, 1.0f };
	vertexData[0].texcoord = { 0.0f, 0.0f };
	// 1: 右上
	vertexData[1].position = { 1.0f, 0.0f, 0.0f, 1.0f };
	vertexData[1].texcoord = { 1.0f, 0.0f };
	// 2: 右下
	vertexData[2].position = { 1.0f, 1.0f, 0.0f, 1.0f };
	vertexData[2].texcoord = { 1.0f, 1.0f };
	// 3: 左下
	vertexData[3].position = { 0.0f, 1.0f, 0.0f, 1.0f };
	vertexData[3].texcoord = { 0.0f, 1.0f };

	vertexResource->Unmap(0, nullptr);

	// BookUi::Initialize 内に追加するコードの例
	auto device = dxCommon_->GetDevice();

	// 256バイトアライメント（定数バッファのルール）に切り上げ
	UINT sizeCB_Curl = (sizeof(PageCurlData) + 0xff) & ~0xff;
	UINT sizeCB_Trans = (sizeof(TransformMatrix) + 0xff) & ~0xff;

	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD; // CPUから書き込めるヒープ

	D3D12_RESOURCE_DESC resDesc{};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// curlDataBufferの生成
	resDesc.Width = sizeCB_Curl;
	device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&curlDataBuffer));

	// transformMatrixBufferの生成
	resDesc.Width = sizeCB_Trans;
	device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&transformMatrixBuffer));

	// *テクスチャ* //

	// 読み込み
	TextureManager::GetInstance()->LoadTexture(textureFilePath_);
	// 番号取得
	textureIndex = TextureManager::GetInstance()->GetSrvIndex(textureFilePath_);

	// 初期状態はページが完全に閉じている状態（curlX = PI）
	currentCurlX_ = 0.0f;
	flipDir = None; // 初期状態は左ページが閉じている状態
	isPageTurnR = false; // 右ページが閉じている状態
	isPageTurnL = false; // 左ページが閉じている状態

	// 影スプライトの初期化
	shadowSprite = std::make_unique<Sprite>();
	shadowSprite->Initialize("Resource/bookUi/shadowL.png"); // 左に落ちる影のテクスチャ
	shadowSprite->SetSize(shadowSize); // 本のサイズに合わせる
	shadowSprite->SetAnchorPoint(Vector2(0.0f, 0.0f));
}

void BookUi::Update() {
	// ページめくりの更新
	UpdatePageTurn();

	if (currentCurlX_ == 0.0f) {
		isOpenPage = true;
	} else {
		isOpenPage = false;
	}

	// === 1. めくりパラメータの更新 ===
	PageCurlData* curlMap = nullptr;
	curlDataBuffer->Map(0, nullptr, reinterpret_cast<void**>(&curlMap));
	curlMap->curlX = currentCurlX_;
	curlMap->isPageTurnR = isPageTurnR;
	curlMap->isPageTurnL = isPageTurnL;
	curlMap->baseZ = baseZ_;
	curlDataBuffer->Unmap(0, nullptr);

	// === 2. 行列の更新 ===
	TransformMatrix* transMap = nullptr;
	transformMatrixBuffer->Map(0, nullptr, reinterpret_cast<void**>(&transMap));

	// 座標位置
	Matrix4x4 matPivot = MakeTranslateMatrix({ anchorPoint.x,anchorPoint.y, 0.0f });

	// --- World行列の計算 ---
	Matrix4x4 matScale = MakeScaleMatrix(scale_);
	Matrix4x4 matRot = MakeRotateMatrix(rotation_);
	Matrix4x4 matTrans = MakeTranslateMatrix(position_);
	Matrix4x4 matWorld = Multiply(matPivot, matScale);
	matWorld = Multiply(matWorld, matRot);
	matWorld = Multiply(matWorld, matTrans);
	
	float kClientWidth = windowAPI_->kClientWidth;
	float kClientHeight = windowAPI_->kClientHeight;

	// 正射影行列の作成（左, 上, 右, 下, 近, 遠）
	Matrix4x4 matProjection = MakeOrthographicMatrix(0.0f, 0.0f, kClientWidth, kClientHeight, 0.0f, 1.0f);

	// World行列 と Projection行列 を掛け合わせる
	// これで「ピクセル座標」が「DirectXの画面座標(-1~1)」に正しく変換されます
	transMap->WorldViewProjectionMatrix = Multiply(matWorld, matProjection);

	transformMatrixBuffer->Unmap(0, nullptr);

	// 進行度を 0.0(待機) ～ 1.0(めくり中) ～ 0.0(めくり終わり) の山なりにする
	float progress = std::clamp(std::abs(currentCurlX_) / PI, 0.0f, 1.0f);

	// sin関数を使って、めくりの中間(progress=0.5)で影が一番濃くなるようにする
	float shadowAlpha = std::sin(progress * PI) * 1.0f; // 0.7f は影の最大濃さ(お好みで)

	float centerX = position_.x;
	float centerY = position_.y;

	if (flipDir == RightOpen) {
		// --- 右ページから左ページへ進める時 ---
		// 影オブジェクト(左に落ちるグラデーション)を有効化
		isShadowActive = true;

		if (shadowSize.x < scale_.x) {
			shadowSize.x += 17.0f;
		}
		shadowSprite->SetSize(shadowSize);

		// 影の透明度を更新
		shadowSprite->SetColor(Vector4(0.0f, 0.0f, 0.0f, shadowAlpha));

		// 影の位置を、本の中央（折り目）に合わせる
		shadowSprite->SetPosition(Vector2(centerX - 30.0f, centerY - (scale_.y / 2.0f)));
	} else if (flipDir == RightClose) {
		// --- 右ページから左ページへ進める時 ---
		// 影オブジェクト(左に落ちるグラデーション)を有効化
		isShadowActive = true;

		if (shadowSize.x > 0.0f) {
			shadowSize.x -= 17.0f;
		}
		shadowSprite->SetSize(shadowSize);

		// 影の透明度を更新
		shadowSprite->SetColor(Vector4(0.0f, 0.0f, 0.0f, shadowAlpha));

		// 影の位置を、本の中央（折り目）に合わせる
		shadowSprite->SetPosition(Vector2(centerX - 30.0f, centerY - (scale_.y / 2.0f)));
	}
	else {
		// --- めくっていない待機時は影を非表示にする ---
		isShadowActive = false;
	}

	shadowSprite->Update();
}

void BookUi::Draw() {
	if (isShadowActive) {
		// スプライト描画用のステート（PSOやRootSignature）に切り替え
		SpriteCommon::GetInstance()->SetCommonPipelineState();

		if (shadowSprite) {
			shadowSprite->Draw();
		}

		// ★ ここが最重要！ ★
		// 影を描画し終わったので、GPUを「ページ描画モード（テッセレーション用）」に戻す
		// おそらく BookUiCommon 等に PSO をセットする関数があるはずです
		BookUiCommon::GetInstance()->SetCommonPipelineState();
	}

	// 1. 定数バッファ(CBV)のセット：めくりパラメータ (b0) と 行列 (b1)
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, curlDataBuffer->GetGPUVirtualAddress()); // b0
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformMatrixBuffer->GetGPUVirtualAddress()); // b1

	// 2. 頂点データ (VBV)
	dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);

	// 3. テクスチャ (SRV) のセット
	dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(textureFilePath_));

	// 4. 描画
	dxCommon_->GetCommandList()->DrawInstanced(4, 1, 0, 0);
}

void BookUi::UpdatePageTurn() {
	if (flipDir == None) return;

	// 終了判定 (PIまでいったら終了)
	if (flipDir == RightOpen || flipDir == LeftOpen) {
		currentCurlX_ -= pageTurnSpeed;

		if (currentCurlX_ < 0.0f) {
			currentCurlX_ = 0.0f;
			flipDir = None;
		}
	} else if (flipDir == RightClose || flipDir == LeftClose) {
		currentCurlX_ += pageTurnSpeed;

		if (currentCurlX_ > DirectX::XM_PI) {
			currentCurlX_ = DirectX::XM_PI;
			flipDir = None;
		}
	}
}

void BookUi::StartOpenPageR() {
	if (flipDir == None) {
		flipDir = RightOpen;
		isPageTurnR = true; // ★右めくり開始
		isPageTurnL = false;
		shadowSize = Vector2(0.0f, scale_.y); // 影のサイズをリセット
	}
}

void BookUi::StartClosePageR() {
	if (flipDir == None) {
		flipDir = RightClose;
		isPageTurnR = true; // ★右めくり開始
		isPageTurnL = false;
		shadowSize = Vector2(scale_.x / 2.0f, scale_.y); // 影のサイズをリセット
	}
}

void BookUi::StartOpenPageL() {
	if (flipDir == None) {
		flipDir = LeftOpen;
		isPageTurnR = false; // ★右めくり開始
		isPageTurnL = true;
	}
}

void BookUi::StartClosePageL() {
	if (flipDir == None) {
		flipDir = LeftClose;
		isPageTurnL = true; // ★左めくり開始
		isPageTurnR = false;
	}
}

void BookUi::ChangeTexture(const std::string& textureFilePath) {
	TextureManager::GetInstance()->LoadTexture(textureFilePath);

	// indexを差し替える
	textureIndex =
		TextureManager::GetInstance()->GetSrvIndex(textureFilePath);
}
