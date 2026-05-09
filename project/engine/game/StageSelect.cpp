
#include <algorithm>
#include <Windows.h>

#include "StageSelect.h"
#include "ObjectCommon.h"
#include "SceneManager.h"
#include "SpriteCommon.h"
#include "BookUiCommon.h"
#include "RadarChartCommon.h"

#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxcompiler.lib")

void StageSelect::Initialize() {
	// カメラ初期化
	camera_ = std::make_unique<Camera>();
	camera_->SetRotate({ cameraTransform.rotate });
	camera_->SetTranslate({ cameraTransform.translate });

	// カメラマネージャ登録
	CameraManager::GetInstance()->AddCamera("main", camera_.get());
	CameraManager::GetInstance()->SetActiveCamera("main");

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


	// レーダーチャート
	radarChart = std::make_unique<RadarChart>();
	radarChart->Initialize();
	radarChart->SetPosition({ radarPosition });
	radarChart->SetMaxRadius(radarChartRadius);
	radarChart->SetColor(radarChartColor);
	radarChart->SetValues(values);
	for (int i = 0; i < kMaxRadarChart; i++) {
		radarChartEasing[i].num = 0.0f;
		radarChartEasing[i].startNumber = 0.0f;
		radarChartEasing[i].endNumber = parameterSetting[i][currentStyle];
		radarChartEasing[i].numberTime = 0.0f;
		radarChartEasing[i].numberEasedT = 0.0f;
	}



	// 本型UI
	std::vector<std::string> textures = {
	"Resource/bookUi/cover.png",
	"Resource/bookUi/cover2.png",
	"Resource/bookUi/normal.png",// normal
	"Resource/bookUi/speed.png",// speed
	"Resource/bookUi/power.png",// power
	"Resource/bookUi/sniper.png",// sniper
	"Resource/bookUi/bookUi_1.png",
	"Resource/bookUi/bookUi_1.png",
	"Resource/bookUi/bookUi_1.png",
	"Resource/bookUi/stage1.png",// stage1
	};
	book = std::make_unique<Book>();
	book->Initialize(textures);

	// 本のイージング
	bookEasing.transform.translate = { 960.0f, 540.0f, 0.0f };
	bookEasing.transform.scale = { 1200.0f, 700.0f, 1.0f };
	bookEasing.startSize = bookEasing.transform.scale;
	bookEasing.endSize = { 3600.0f, 2100.0f, 1.0f };
	bookEasing.sizeTime = 0.0f;
	bookEasing.sizeEasedT = 0.0f;

	book->SetPosition(bookEasing.transform.translate);
	book->SetScale(bookEasing.transform.scale);

}

void StageSelect::Update() {
	// 入力取得
	auto input = Input::GetInstance();
	// カメラ更新
	CameraManager::GetInstance()->Update();

	transform_.rotate.y += 0.05f;
	playerObject_->SetRotate(transform_.rotate);
	playerObject_->SetTranslate(transform_.translate);

	// 切り換えクールタイム減少
	switchCooltime = (std::max)(0.0f, switchCooltime - 1.0f / 60.0f);
	if (isStageSelect) {
		if (switchCooltime <= 0.0f) {
			book->NextPage();
			switchCooltime = 0.3f; // クールタイムリセット
		}
	}
	if (switchCooltime <= 0.0f) {
		// パラメータのイージングセット
		if (isParameterEasing) {
			isParameterEasing = false;
			ParameterEasingSet(currentStyle);
		}
	}


	bool isChanged = false; // マシン変更
	if (input->TriggerKey(DIK_D) || input->TriggerKey(DIK_RIGHT)) {
		if (switchCooltime <= 0.0f) {
			if (currentStyle != sniper) {
				if (!isStageSelect) {
					currentStyle = static_cast<Style>((static_cast<int>(currentStyle) + 1) % 4);
					isChanged = true;

					// 本のページをめくる
					if (book->GetCurrentPageIndex() < 5)
					book->NextPage();

					isParameterEasing = true; // イージングリセット
					ParameterEasingSet(currentStyle);

				} else {
					if (currentStage < 5) {
						currentStage++;
					} else {
						currentStage = 0;
					}

					// 本のページをめくる
					book->NextPage();

					isParameterEasing = true; // イージングリセット
					ParameterEasingSet(currentStyle);

				}
				switchCooltime = 0.8f; // クールタイムリセット
			}
		}
	} else if (input->TriggerKey(DIK_A) || input->TriggerKey(DIK_LEFT)) {
		if (switchCooltime <= 0.0f) {
			if (currentStyle != normal) {
				if (!isStageSelect) {
					currentStyle = static_cast<Style>((static_cast<int>(currentStyle) + 3) % 4);
					isChanged = true;

					// 本のページを戻す
					book->PrevPage();

					isParameterEasing = true; // イージングリセット
					ParameterEasingSet(currentStyle);

				} else {
					if (currentStage > 0) {
						currentStage--;
					} else {
						currentStage = 5;
					}

					if (book->GetCurrentPageIndex() > 8) {
						// 本のページを戻す
						book->PrevPage();

						isParameterEasing = true; // イージングリセット
						ParameterEasingSet(currentStyle);
					}

				}
				switchCooltime = 0.8f; // クールタイムリセット
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
			//SceneManager::GetInstance()->ChangeScene("GAMEPLAY");

			// シーン切り替え演出
			isTransition = true;
			isFullScreenCA = true;
			isSpeedDistortion = true;
			isRadialBlur = true;

		} else {
			isStageSelect = true;
			isParameterEasing = true; // イージングリセット
			ParameterEasingSet(currentStyle);
		}
	}

	if (!isStageSelect) {
		playerObject_->Update();
	}

	// 本の更新
	book->Update();
	// シーン切り替え演出
	TransitionUpdate();
	radarChart->Update();


	// レーダーチャート

	float radarValues[5];
	for (int i = 0; i < kMaxRadarChart; i++) {
		if (!isParameterEasing)
			easing->Number(radarChartEasing[i], 0.01f, 0);
		else
			easing->Number(radarChartEasing[i], 0.05f, 0);

		radarValues[i] = radarChartEasing[i].num;
		radarChart->SetValues(radarValues);
	}


	//イージング
	easing->Update();
	easing->Draw();

	LithingEffect();
}
void StageSelect::Draw2D() {
	// 2Dオブジェクトの描画準備
	SpriteCommon::GetInstance()->SetCommonPipelineState();

	//if (switchCooltime <= 0.0f) {
	//	for (int i = 0; i < kMaxParameter; i++) {
	//		parameterGauge[i]->Draw();
	//		parameter[i]->Draw();
	//	}
	//}

	RadarChartCommon::GetInstance()->SetCommonPipelineState();

	radarChart->Draw();


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

	// 本型UIの描画準備
	BookUiCommon::GetInstance()->SetCommonPipelineState();

	book->Draw();

}

void StageSelect::Finalize() { CameraManager::GetInstance()->RemoveCamera("main"); }

void StageSelect::TransitionUpdate() {
	if (isTransition) {
		easing->Size(bookEasing, 0.01f, 1);

		if (bookEasing.sizeEasedT >= 0.1f) {
			if(fullScreenCAIntensity < 1.0f)
				fullScreenCAIntensity += 0.01f;
			if(speedDistortionStrength < 1.0f)
				speedDistortionStrength += 0.1f;
			if(blurWidth <= 0.01f)
				blurWidth += 0.0001f;
			if(bloomThreshold > 0.0f)
				bloomThreshold -= 0.01f;
			if (bloomIntensity < 10.0f)
				bloomIntensity += 0.01f;

		}

		// 本のサイズを更新
		book->SetScale(bookEasing.transform.scale);

		if(bookEasing.sizeEasedT >= 1.0f) {
			// シーン切り替え
			SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
		}

	}
}

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
	PostEffect::GetInstance()->SetBokehRadius(bloomBlurRadius);
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
	// 色収差
	PostEffect::GetInstance()->SetFullScreenCA(isFullScreenCA);
	PostEffect::GetInstance()->SetFullScreenCAIntensity(fullScreenCAIntensity);
	// スピードディストーション
	PostEffect::GetInstance()->SetSpeedDistortion(isSpeedDistortion);
	PostEffect::GetInstance()->SetSpeedDistortionStrength(speedDistortionStrength);

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
	for (int i = 0; i < kMaxRadarChart; i++) {
		if (isParameterEasing) {
			radarChartEasing[i].startNumber = radarChartEasing[i].num;
			radarChartEasing[i].endNumber = 0.0f;
			radarChartEasing[i].numberTime = 0.0f;
			radarChartEasing[i].numberEasedT = 0.0f;
		} else {
			radarChartEasing[i].num = 0.0f;
			radarChartEasing[i].startNumber = 0.0f;
			radarChartEasing[i].endNumber = parameterSetting[i][currentStyle];
			radarChartEasing[i].numberTime = 0.0f;
			radarChartEasing[i].numberEasedT = 0.0f;
		}

	}
}
