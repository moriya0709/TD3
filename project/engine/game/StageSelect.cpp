#include "StageSelect.h"
#include "ObjectCommon.h"
#include "SceneManager.h"
#include "SpriteCommon.h"

#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxcompiler.lib")

void StageSelect::Initialize() {
	// カメラ初期化
	camera_ = std::make_unique<Camera>();
	camera_->SetRotate({cameraTransform.rotate});
	camera_->SetTranslate({cameraTransform.translate});

	// カメラマネージャ登録
	CameraManager::GetInstance()->AddCamera("main", camera_.get());
	CameraManager::GetInstance()->SetActiveCamera("main");

	// パラメータ
	for(int i = 0; i< kMaxParameter; i++){
		parameter[i] = std::make_unique<Sprite>();
		parameter[i]->Initialize("Resource/parameters/parameters.png");
		parameter[i]->SetPosition({ 1300.0f, 800.0f + i * 60.0f });
		parameter[i]->SetSize({ 500.0f, 50.0f });
		parameter[i]->SetAnchorPoint({ 0.0f, 0.0f });
		parameterGauge[i] = std::make_unique<Sprite>();
		parameterGauge[i]->Initialize("Resource/parameters/parametersGauge.png");
		parameterGaugeEasing[i].pos = { 1300.0f, 800.0f + i * 60.0f };
		parameterGaugeEasing[i].size = { 0.0f, 50.0f };
		parameterGaugeEasing[i].startSizeV2 = { 0.0f, 50.0f };
		parameterGaugeEasing[i].endSizeV2 = { float(parameterSetting[i][currentStyle] * 5), parameterGaugeEasing[i].size.y};
		parameterGaugeEasing[i].sizeTime = 0.0f;
		parameterGaugeEasing[i].sizeEasedT = 0.0f;
		parameterGauge[i]->SetPosition(parameterGaugeEasing[i].pos);
		parameterGauge[i]->SetSize(parameterGaugeEasing[i].size);
		parameterGauge[i]->SetAnchorPoint({ 0.0f, 0.0f });
	}

	// イージング
	easing = std::make_unique<Easing>();
	easing->Initialize();

	playerObject_ = std::make_unique<Object>();
	playerObject_->Initialize(camera_.get());
	switch (currentStyle) {
	case normal:
		playerObject_->SetModel("normalMachine.obj");
		break;
	case speed:
		playerObject_->SetModel("speedMachine.obj");
		break;
	case power:
		playerObject_->SetModel("powerMachine.obj");
		break;
	case sniper:
		playerObject_->SetModel("sniperMachine.obj");
		break;
	default:
		playerObject_->SetModel("normalMachine.obj");
		break;
	}

	playerObject_->SetTranslate(transform_.translate);
}

void StageSelect::Update() {
	// 入力取得
	auto input = Input::GetInstance();
	// カメラ更新
	CameraManager::GetInstance()->Update();

	transform_.rotate.y += 0.05f;
	playerObject_->SetRotate(transform_.rotate);
	playerObject_->SetTranslate(transform_.translate);

	bool isChanged = false; // マシン変更
	if (input->TriggerKey(DIK_D) || input->TriggerKey(DIK_RIGHT)) {
		if (!isStageSelect) {
			currentStyle = static_cast<Style>((static_cast<int>(currentStyle) + 1) % 4);
			isChanged = true;

			// パラメータのイージングセット
			ParameterEasingSet(currentStyle);
		} else {
			if (currentStage < 5) {
				currentStage++;
			} else {
				currentStage = 0;
			}
		}
	} else if (input->TriggerKey(DIK_A) || input->TriggerKey(DIK_LEFT)) {
		if (!isStageSelect) {
			currentStyle = static_cast<Style>((static_cast<int>(currentStyle) + 3) % 4);
			isChanged = true;

			// パラメータのイージングセット
			ParameterEasingSet(currentStyle);
		} else {
			if (currentStage > 0) {
				currentStage--;
			} else {
				currentStage = 5;
			}
		}
	}
	if (!isStageSelect) {
		if (isChanged) {
			switch (currentStyle) {
			case normal:
				playerObject_->SetModel("normalMachine.obj");
				break;
			case speed:
				playerObject_->SetModel("speedMachine.obj");
				break;
			case power:
				playerObject_->SetModel("powerMachine.obj");
				break;
			case sniper:
				playerObject_->SetModel("sniperMachine.obj");
				break;
			}
		}
	}

	// ENTERキーを押したら
	if (input->TriggerKey(DIK_RETURN)) {
		// ゲームプレイシーン(次シーン)を生成
		if (isStageSelect) {
			SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
		} else {
			isStageSelect = true;
		}
	}

	if (!isStageSelect) {
		playerObject_->Update();
	}

	for (int i = 0; i < kMaxParameter; i++) {
		if(parameterGaugeEasing[i - 1].sizeTime >= 0.5f || i == 0)
			easing->SizeV2(parameterGaugeEasing[i], 0.05f, 0);

		parameterGauge[i]->SetSize(parameterGaugeEasing[i].size);

		parameterGauge[i]->Update();
		parameter[i]->Update();
	}

	LithingEffect();
}
void StageSelect::Draw2D() {
	// 2Dオブジェクトの描画準備
	SpriteCommon::GetInstance()->SetCommonPipelineState();

	for (int i = 0; i < kMaxParameter; i++) {
		parameterGauge[i]->Draw();
		parameter[i]->Draw();
	}
}

void StageSelect::Draw3D() {
	// 3Dオブジェクトの描画準備
	ObjectCommon::GetInstance()->SetCommonDrawSetting();

	ObjectCommon::GetInstance()->SetCommonPipelineState();
	// 3Dオブジェクト描画
	if (!isStageSelect) {
		if (playerObject_) {
			playerObject_->Draw();
		}
	}
}

void StageSelect::Finalize() { CameraManager::GetInstance()->RemoveCamera("main"); }

void StageSelect::LithingEffect() {
#pragma region ポストエフェクト

	// *ポストエフェクト* //
	PostEffect::GetInstance()->Update(camera_.get());

	// 反転
	PostEffect::GetInstance()->SetInversion(isInversion);
	// グレースケール
	PostEffect::GetInstance()->SetGrayscale(isGrayscale);
	// 放射線ブラー
	PostEffect::GetInstance()->SetRadialBlur(isRadialBlur);
	PostEffect::GetInstance()->SetBlurCenter(blurCenter);
	PostEffect::GetInstance()->SetBlurWidth(blurWidth);
	PostEffect::GetInstance()->SetBlurSamples(blurSamples);
	// ディスタンスフォグ
	PostEffect::GetInstance()->SetDistanceFog(isDistanceFog);
	PostEffect::GetInstance()->SetDistanceFogColor(distanceFogColor);
	PostEffect::GetInstance()->SetDistanceFogStart(distanceStart);
	PostEffect::GetInstance()->SetDistanceFogEnd(distanceEnd);
	// ハイトフォグ
	PostEffect::GetInstance()->SetHeightFog(isHeightFog);
	PostEffect::GetInstance()->SetHeightFogColor(heightFogColor);
	PostEffect::GetInstance()->SetHeightFogTop(heightFogTop);
	PostEffect::GetInstance()->SetHeightFogBottom(heightFogBottom);
	PostEffect::GetInstance()->SetHeightFogDensity(heightFogDensity);
	PostEffect::GetInstance()->HightFogUpdate(camera_.get());
	// DOF
	PostEffect::GetInstance()->SetDOF(isDOF);
	PostEffect::GetInstance()->SetFocusDistance(focusDistance);
	PostEffect::GetInstance()->SetBokehRadius(bokehRadius);
	PostEffect::GetInstance()->SetFocusRange(focusRange);
	// ブルーム
	PostEffect::GetInstance()->SetBloomIntensity(bloomIntensity);
	PostEffect::GetInstance()->SetBloomThreshold(bloomThreshold);
	// レンズフレア
	PostEffect::GetInstance()->SetLensFlare(isLensFlare);
	PostEffect::GetInstance()->SetLensFlareGhostCount(lensFlareGhostCount);
	PostEffect::GetInstance()->SetLensFlareHaloWidth(lensFlareHaloWidth);
	PostEffect::GetInstance()->SetIsACES(isACES);
	PostEffect::GetInstance()->SetCAIntensity(caIntensity);
	// モーションブラー
	PostEffect::GetInstance()->SetMotionBlur(isMotionBlur);
	PostEffect::GetInstance()->SetMotionBlurSamples(motionBlurSamples);
	PostEffect::GetInstance()->SetMotionBlurScale(motionBlurScale);

#pragma endregion

#pragma region レイマーチング
	// レイマーチング
	RayMarching::GetInstance()->Update(camera_.get());
	// rayMarching->SetTime(rayMarchingTime);
	RayMarching::GetInstance()->SetSunDir(rayMarchingSunDir);
	RayMarching::GetInstance()->SetCloudCoverage(rayMarchingCloudCoverage);
	RayMarching::GetInstance()->SetCloudTop(rayMarchingCloudBottom);
	RayMarching::GetInstance()->SetCloudBottom(rayMarchingCloudTop);
	RayMarching::GetInstance()->SetRialLight(rayMarchingIsRialLight);
	RayMarching::GetInstance()->SetAnimeLight(rayMarchingIsAnimeLight);
	RayMarching::GetInstance()->SetMotionBlur(rayMarchingIsMotionBlur);
	RayMarching::GetInstance()->SetCloudOpacity(rayMarchingCloudOpacity);

#pragma endregion
}

void StageSelect::ParameterEasingSet(Style currentStyle) {
	for (int i = 0; i < kMaxParameter; i++) {
		parameterGaugeEasing[i].size = { 0.0f, 50.0f };
		parameterGaugeEasing[i].startSizeV2 = { 0.0f,parameterGaugeEasing[i].size.y};
		parameterGaugeEasing[i].endSizeV2 = { float(parameterSetting[i][currentStyle] * 5),parameterGaugeEasing[i].size.y};
		parameterGaugeEasing[i].sizeTime = 0.0f;
		parameterGaugeEasing[i].sizeEasedT = 0.0f;

		parameterGauge[i]->SetSize(parameterGaugeEasing[i].size);
	}
}
