
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

	// レーダーチャート
	radarChart = std::make_unique<RadarChart>();
	radarChart->Initialize();
	radarChart->SetkVertices(kMaxRadarChart);
	radarChart->SetValues(values);
	radarChart->SetPosition({ radarPosition });
	radarChart->SetMaxRadius(radarChartRadius);
	radarChart->SetColor(radarChartColor);
	for (int i = 0; i < kMaxRadarChart; i++) {
		radarChartEasing[i].num = 0.0f;
		radarChartEasing[i].startNumber = 0.0f;
		radarChartEasing[i].endNumber = parameterSetting[i][currentStyle];
		radarChartEasing[i].numberTime = 0.0f;
		radarChartEasing[i].numberEasedT = 0.0f;
	}

	return_ = std::make_unique<Sprite>();
	return_->Initialize("Resource/return.png"); // Wで戻る
	return_->SetPosition({ 950.0f, 100.0f });

	space_ = std::make_unique<Sprite>();
	space_->Initialize("Resource/space.png"); // space進める
	space_->SetPosition({ 950.0f, 1000.0f });

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
	"Resource/bookUi/stage2.png",// stage2
	"Resource/bookUi/stage3.png",// stage3
	"Resource/bookUi/stage4.png",// stage4
	"Resource/bookUi/stage5.png",// stage5
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

	// 音声再生
	SoundManager::GetInstance()->Play("select.mp3");
	//再生フラグ
	isSelectBGMPlaying_ = false;
}

void StageSelect::Update() {
	// 入力取得
	auto input = Input::GetInstance();
	// カメラ更新
	CameraManager::GetInstance()->Update();

	if (!isSelectBGMPlaying_) {
		SoundManager::GetInstance()->Play("select.mp3", true);
		isSelectBGMPlaying_ = true;
	}

	// 切り換えクールタイム減少
	switchCooltime = (std::max)(0.0f, switchCooltime - 1.0f / 60.0f);
	if (isStageSelect) {
		if (switchCooltime <= 0.0f) {
			if (book->GetCurrentPageIndex() < 9) {
				book->NextPage();
				switchCooltime = 0.3f; // クールタイムリセット
			}
		}
	} else {
		if (switchCooltime <= 0.0f)
		{//マシーンセレクトからステージセレクトへ戻す
			if (book->GetCurrentPageIndex() > 5)
			{
				book->PrevPage();
				switchCooltime = 0.3f;
			}
		}
		if (switchCooltime <= 0.0f) {
			// パラメータのイージングセット
			if (isParameterEasing) {
				isParameterEasing = false;
				ParameterEasingSet(currentStyle);
			}
		}
	}


	if (!isTransition) {
		if (input->PushKey(DIK_D) || input->PushKey(DIK_RIGHT)) {
			if (switchCooltime <= 0.0f) {
				if (!isStageSelect) {
					// 本のページをめくる
					if (book->GetCurrentPageIndex() < 5) {
						book->NextPage();

						// スタイルを切り替える
						currentStyle = static_cast<Style>((static_cast<int>(currentStyle) + 1) % 4);
						isParameterEasing = true; // イージングリセット
						ParameterEasingSet(currentStyle);
					}

				} else {
					if (currentStage < 5) {
						currentStage++;
					}

					// 本のページをめくる
					if (book->GetCurrentPageIndex() < 13)
						book->NextPage();

				}
				switchCooltime = 0.8f; // クールタイムリセット
			}
		} else if (input->PushKey(DIK_A) || input->PushKey(DIK_LEFT)) {
			if (switchCooltime <= 0.0f) {
				if (!isStageSelect) {
					if (book->GetCurrentPageIndex() > 2) {
						// スタイルを切り替える
						currentStyle = static_cast<Style>((static_cast<int>(currentStyle) + 3) % 4);

						// 本のページを戻す
						book->PrevPage();


						isParameterEasing = true; // イージングリセット
						ParameterEasingSet(currentStyle);
					}

				} else {
					if (currentStage > 1) {
						currentStage--;
					}

					if (book->GetCurrentPageIndex() > 9) {
						// 本のページを戻す
						book->PrevPage();
					}

				}
				switchCooltime = 0.8f; // クールタイムリセット
			}
		}

		// ENTERキーを押したら
		if (input->TriggerKey(DIK_SPACE)) {
			// ゲームプレイシーン(次シーン)を生成
			if (isStageSelect) {
				if (book->GetCurrentPageIndex() > 8) {
					// シーン切り替え演出
					isTransition = true;
					isSpeedDistortion = true;
					isRadialBlur = true;

					if(currentStage == 1)
						rayMarchingSunDir = { 0.3f, -0.5f, 0.2f };
					else if(currentStage == 2)
						rayMarchingSunDir = { -0.34f, -0.15f, -1.0f };
					else if(currentStage == 3)
						rayMarchingSunDir = { 0.0f, 0.01f, -1.0f };
					else if(currentStage == 4)
						rayMarchingSunDir = { 0.12f, 0.05f, 1.0f };
					else if(currentStage == 5)
						rayMarchingSunDir = { -0.42f, -0.33f, -1.0f };

				}

			} else {
				isStageSelect = true;
				isParameterEasing = true; // イージングリセット
				ParameterEasingSet(currentStyle);
			}
		}

		//タイトルに戻す
		if (input->TriggerKey(DIK_W)) {
			if (switchCooltime <= 0.0f) {
				if (isStageSelect) {//ステージセレクトならマシーンセレクトに(演出)
					isStageSelect = false;
					isParameterEasing = true;
					ParameterEasingSet(currentStyle);
					switchCooltime = 0.5f;
				} else {
					isBackTransition = true;
					return;
				}
			}
		}
	}

	if (isBackTransition) {
		intensity = (std::max)(0.0f, intensity - 1.0f / 30.0f);

		if (intensity <= 0.0f) {
			// シーン切り替え処理
			SoundManager::GetInstance()->Stop("select.mp3");
			isSelectBGMPlaying_ = false;
			SceneManager::GetInstance()->ChangeScene("TITLE");
		}
	} else {
		if (intensity <= 1.0f)
			intensity += 1.0f / 30.0f; // 1秒かけて明るくする
	}

	// 本の更新
	book->Update();
	// シーン切り替え演出
	TransitionUpdate();
	radarChart->Update();

	return_->Update();
	space_->Update();

	// レーダーチャート

	// 1. あらかじめ最大要素数でリサイズしておく
	std::vector<float> radarValues(kMaxRadarChart);

	for (int i = 0; i < kMaxRadarChart; i++) {
		if (!isParameterEasing)
			easing->Number(radarChartEasing[i], 0.01f, 0);
		else
			easing->Number(radarChartEasing[i], 0.05f, 0);

		// 2. 確保済みの要素に代入
		radarValues[i] = radarChartEasing[i].num;
	}

	// 3. 全ての値が決まった後に、ループの外で 1回だけ呼び出す
	radarChart->SetValues(radarValues);


	//イージング
	easing->Update();
	easing->Draw();

	LithingEffect();
}
void StageSelect::Draw2D() {
	// 2Dオブジェクトの描画準備
	SpriteCommon::GetInstance()->SetCommonPipelineState();

	return_->Draw();
	space_->Draw();

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


	// 本型UIの描画準備
	BookUiCommon::GetInstance()->SetCommonPipelineState();

	book->Draw();

}

void StageSelect::Finalize() { CameraManager::GetInstance()->RemoveCamera("main"); }

void StageSelect::TransitionUpdate() {
	if (isTransition) {
		easing->Size(bookEasing, 0.01f, 1);

		if (bookEasing.sizeEasedT >= 0.1f) {
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
			SoundManager::GetInstance()->Stop("select.mp3");
			isSelectBGMPlaying_ = false;
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
	PostEffect::GetInstance()->SetTwoColor(isTwoColor);
	PostEffect::GetInstance()->SetThreshold(threshold);
	PostEffect::GetInstance()->SetContrast(contrast);
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
	// スピードディストーション
	PostEffect::GetInstance()->SetSpeedDistortion(isSpeedDistortion);
	PostEffect::GetInstance()->SetSpeedDistortionStrength(speedDistortionStrength);
	// 集中線
	PostEffect::GetInstance()->SetConcentrationLines(isConcentrationLines);
	PostEffect::GetInstance()->SetConcentrationLineIntensity(concentrationLineIntensity);
	PostEffect::GetInstance()->SetConcentrationLineCenter(concentrationLineCenter);
	PostEffect::GetInstance()->SetConcentrationLineDensity(concentrationLineDensity);
	PostEffect::GetInstance()->SetConcentrationLineLength(concentrationLineLength);
	PostEffect::GetInstance()->SetConcentrationLineSpeed(concentrationLineSpeed);
	// エフェクトの強さ
	PostEffect::GetInstance()->SetIntensity(intensity);

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
