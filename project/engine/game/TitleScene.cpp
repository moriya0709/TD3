#include "TitleScene.h"
#include "ObjectCommon.h"
#include "SpriteCommon.h"
#include "BookUiCommon.h"
#include "SceneManager.h"
#include "Model.h"
#include "RadarChartCommon.h"

void TitleScene::Initialize() {

	// カメラ初期化
	camera = std::make_unique<Camera>();
	camera->SetRotate({ cameraTransform.rotate });
	camera->SetTranslate({ cameraTransform.translate });

	// カメラマネージャ登録
	CameraManager::GetInstance()->AddCamera("main", camera.get());
	CameraManager::GetInstance()->SetActiveCamera("main");

	// スプライト
	title_ = std::make_unique <Sprite>();
	title_->Initialize("Resource/titleF.png");
	title_->SetPosition({ 900.0f, 300.0f }); // 画面中央

	// 3Dオブジェクト
	for (int i = 0; i < 2; i++) {
		object[i] = std::make_unique <Object>();
		object[i]->Initialize(camera.get());
	}

	///
	///アニメーションモデル
	/// 

	//スケルトン
	Model* model = ModelManager::GetInstance()->FindModel("simpleSkin.gltf");//スケルトンアクセス権
	skeleton_ = model->CreateSkeleton(model->GetModelData().rootNode);//動く仕組み

	//アニメーションデータの読み込み(モデル自体はGame.cppに入れること)
	simpleAnimation_ = Model::LoadAnimationFile("./Resource", "simpleSkin.gltf");//スケルトン
	walkAnimation_ = Model::LoadAnimationFile("./Resource", "walk.gltf");

	// アニメーション用オブジェクトの生成と設定
	auto walkAnim = std::make_unique<Object>();
	walkAnim->Initialize(camera.get()); // 初期化
	walkAnim->SetModel("walk.gltf", true); // アニメーションは「true」を入れること
	walkAnim->SetScale({ 1.0f, 1.0f, 1.0f });
	walkAnim->SetRotate({ 0.0f, 0.0f, 0.0f });
	walkAnim->SetTranslate({ 5.0f, -1.0f, 5.0f });

	walkAnim->PlayAnimation(walkAnimation_);//アニメーション読み込み
	walkAnimation = walkAnim.get();//アニメーション読み込み

	animationObjects.push_back(std::move(walkAnim));//アニメーションモデル専用のリストに入れる

	// ヒットエフェクト
	for (int i = 0; i < hitEffectCount; i++) {
		hitEffect[i] = std::make_unique<ParticleEmitter>();
		hitEffect[i]->Initialize("hitEffect1", Transform{}, 5, 0.2f);
	}
	hitEffect[0]->SetActive("hitEffect1");
	hitEffect[0]->LoadParticle("Resource/particle/hit_1.csv");
	hitEffect[1]->SetActive("hitEffect2");
	hitEffect[1]->LoadParticle("Resource/particle/hit_2.csv");
	hitEffect[2]->SetActive("hitEffect3");
	hitEffect[2]->LoadParticle("Resource/particle/hit_3.csv");
	hitEffect[3]->SetActive("hitEffect4");
	hitEffect[3]->LoadParticle("Resource/particle/hit_4.csv");

	// パーティクル
	particleEmitter = std::make_unique<ParticleEmitter>();
	particleEmitter->Initialize("Special3", transformParticle, 5, 0.1f);
	particleEmitter->SetActive("Special3");
	particleEmitter->LoadParticle("Resource/particle/special_2.csv");

	// 初期化済みの3Dオブジェクトにモデルを紐づける
	object[0]->SetModel("emission.obj");
	object[1]->SetModel("skydome.obj");

	// 音声再生
	//SoundManager::GetInstance()->Play("bgm");

}

void TitleScene::Update() {
	// 入力取得
	auto input = Input::GetInstance();
	// カメラ更新
	CameraManager::GetInstance()->Update();

	// ENTERキーを押したら
	if (input->TriggerKey(DIK_RETURN)) {
		// ゲームプレイシーン(次シーン)を生成
		SceneManager::GetInstance()->ChangeScene("GAMESELECT");
	}

	// * 3Dオブジェクト* //
	for (int i = 0; i < 2; i++) {
		object[i]->Update();
	}

	//アニメーションするモデル更新処理
	for (auto& object : animationObjects) {
		object->Update();
	}

	if (!animationObjects.empty()) {
		Object* animationObject = animationObjects[0].get(); // アニメーションモデルを取得

		//アニメーションするモデル更新処理
		if (animationObject->IsSkeletal()) {
			Vector3 scale = animationObject->GetScale();
			Vector3 rotate = animationObject->GetRotate();
			Vector3 translate = animationObject->GetTranslate();

			// アニメーションモデルのワールド行列を作る
			Matrix4x4 animationWorldMatrix = MakeAffineMatrix(scale, rotate, translate);
		}
	}

	// ヒットエフェクト更新
	//for (int i = 0; i < hitEffectCount; i++) {
	//	hitEffect[i]->Update();
	//}

	// パーティクル更新
	particleEmitter->Update();
	particleEmitter->Editor();


	// *スプライト* //
	// sprite更新
	//title_->Update();


#pragma region ライティング
	// *ライティング* //
	for (int i = 0; i < 2; i++) {
		// 平行光
		object[i]->SetDirectionalLight(isDirectionalLight);
		object[i]->SetDirectionalLightDirection(DirectionalLightDirection);
		object[i]->SetDirectionalLightColor(DirectionalLightColor);
		object[i]->SetDirectionalLightIntensity(DirectionalLightIntensity);
		// 環境光
		object[i]->SetAmbientLight(isAmbientLight);
		object[i]->SetAmbientLightColor(AmbientLightColor);
		object[i]->SetAmbientLightIntensity(AmbientLightIntensity);
		// ポイントライト
		object[i]->SetPointLight(isPointLight);
		object[i]->SetPointLightColor(PointLightColor);
		object[i]->SetPointLightPosition(PointLightPosition);
		object[i]->SetPointLightIntensity(PointLightIntensity);
		// スポットライト
		object[i]->SetSpotLight(isSpotLight);
		object[i]->SetSpotLightColor(SpotLightColor);
		object[i]->SetSpotLightPosition(SpotLightPosition);
		object[i]->SetSpotLightDirection(SpotLightDirection);
		object[i]->SetSpotLightRange(SpotLightRange);
		object[i]->SetSpotLightIntensity(SpotLightIntensity);
	}
#pragma endregion

#pragma region ポストエフェクト
	// *ポストエフェクト* //
	PostEffect::GetInstance()->Update(camera.get());

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
	PostEffect::GetInstance()->HightFogUpdate(camera.get());
	// DOF
	PostEffect::GetInstance()->SetDOF(isDOF);
	PostEffect::GetInstance()->SetFocusDistance(focusDistance);
	PostEffect::GetInstance()->SetBokehRadius(bokehRadius);
	PostEffect::GetInstance()->SetFocusRange(focusRange);
	// ブルーム
	PostEffect::GetInstance()->SetBloomIntensity(bloomIntensity);
	PostEffect::GetInstance()->SetBloomThreshold(bloomThreshold);
	PostEffect::GetInstance()->SetBloomBlurRadius(bloomBlurRadius);
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
	// 集中線
	PostEffect::GetInstance()->SetConcentrationLines(isConcentrationLines);
	PostEffect::GetInstance()->SetConcentrationLineIntensity(concentrationLineIntensity);
	PostEffect::GetInstance()->SetConcentrationLineCenter(concentrationLineCenter);
	PostEffect::GetInstance()->SetConcentrationLineDensity(concentrationLineDensity);
	PostEffect::GetInstance()->SetConcentrationLineLength(concentrationLineLength);
	PostEffect::GetInstance()->SetConcentrationLineSpeed(concentrationLineSpeed);
	// ピンチエフェクト
	PostEffect::GetInstance()->SetPinch(isPinchEffect);
	PostEffect::GetInstance()->SetPinchStrength(pinchStrength);
	PostEffect::GetInstance()->SetPinchCenter(pinchCenter);
	PostEffect::GetInstance()->SetPinchRadius(pinchRadius);

#pragma endregion

#pragma region レイマーチング
	// レイマーチング
	RayMarching::GetInstance()->Update(camera.get());
	//rayMarching->SetTime(rayMarchingTime);
	RayMarching::GetInstance()->SetSunDir(rayMarchingSunDir);
	RayMarching::GetInstance()->SetCloudCoverage(rayMarchingCloudCoverage);
	RayMarching::GetInstance()->SetCloudTop(rayMarchingCloudBottom);
	RayMarching::GetInstance()->SetCloudBottom(rayMarchingCloudTop);
	RayMarching::GetInstance()->SetRialLight(rayMarchingIsRialLight);
	RayMarching::GetInstance()->SetAnimeLight(rayMarchingIsAnimeLight);
	RayMarching::GetInstance()->SetMotionBlur(rayMarchingIsMotionBlur);
	RayMarching::GetInstance()->SetCloudOpacity(rayMarchingCloudOpacity);
	RayMarching::GetInstance()->SetStorm(isStorm);
	RayMarching::GetInstance()->SetThunderFrequency(thunderFrequency);
	RayMarching::GetInstance()->SetThunderBrightness(thunderBrightness);


#pragma endregion

#ifdef USE_IMGUI
	// ImGui
	// フレームレートの取得と表示
	float fps = ImGui::GetIO().Framerate;
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / fps, fps);

	ImGui::DragFloat3("cameraTranslate", &cameraTransform.translate.x, 0.1f, -500.0f, 500.0f);
	ImGui::DragFloat3("cameraRotate", &cameraTransform.rotate.x, 0.01f, -10.0f, 10.0f);
	camera->SetTranslate(cameraTransform.translate);
	camera->SetRotate(cameraTransform.rotate);

#pragma region ライティング
	// *ライティング* //
	ImGui::Text("Lighting"); // ライティングのテキスト
	
	// 平行光
	if (ImGui::TreeNode("DirectionalLight")) {
		ImGui::Checkbox("OnOff", &isDirectionalLight);
		if (isDirectionalLight) {
			ImGui::ColorEdit4("Color", &DirectionalLightColor.x);
			ImGui::DragFloat3("Direction", &DirectionalLightDirection.x, 0.01f, -100.0f, 100.0f);
			ImGui::DragFloat("Intensity", &DirectionalLightIntensity, 0.01f, 0.0f, 10.0f);
		}
		ImGui::TreePop();
	}
	// 環境光
	if (ImGui::TreeNode("AmbientLight")) {
		ImGui::Checkbox("OnOff", &isAmbientLight);
		if (isAmbientLight) {
			ImGui::ColorEdit4("Color", &AmbientLightColor.x);
			ImGui::DragFloat("Intensity", &AmbientLightIntensity, 0.01f, 0.0f, 10.0f);
		}

		ImGui::TreePop();
	}
	// ポイントライト
	if (ImGui::TreeNode("PointLight")) {
		ImGui::Checkbox("OnOff", &isPointLight);
		if (isPointLight) {
			ImGui::ColorEdit4("Color", &PointLightColor.x);
			ImGui::DragFloat3("Position", &PointLightPosition.x, 0.01f, -100.0f, 100.0f);
			ImGui::DragFloat("Intensity", &PointLightIntensity, 0.01f, 0.0f, 10.0f);
		}

		ImGui::TreePop();
	}
	// スポットライト
	if (ImGui::TreeNode("SpotLight")) {
		ImGui::Checkbox("OnOff", &isSpotLight);
		if (isSpotLight) {
			ImGui::ColorEdit4("Color", &SpotLightColor.x);
			ImGui::DragFloat3("Position", &SpotLightPosition.x, 0.01f, -100.0f, 100.0f);
			ImGui::DragFloat3("Direction", &SpotLightDirection.x, 0.01f, -100.0f, 100.0f);
			ImGui::DragFloat("Range", &SpotLightRange, 0.01f, 0.0f, 100.0f);
			ImGui::DragFloat("Intensity", &SpotLightIntensity, 0.01f, 0.0f, 10.0f);
		}

		ImGui::TreePop();
	}

#pragma endregion

#pragma region ポストエフェクト
	// *ポストエフェクト* //
	ImGui::Text("PostEffect"); // ポストエフェクトのテキスト

	// 反転
	if (ImGui::TreeNode("inversion")) {
		ImGui::Checkbox("OnOff", &isInversion);

		ImGui::TreePop();
	}
	// グレースケール
	if (ImGui::TreeNode("grayscale")) {
		ImGui::Checkbox("OnOff", &isGrayscale);
		if (isGrayscale) {
			ImGui::Checkbox("isTwoColor", &isTwoColor);
			ImGui::DragFloat("threshold", &threshold, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("contrast", &contrast, 0.01f, 0.0f, 10.0f);
		}

		ImGui::TreePop();
	}
	// 放射線ブラー
	if (ImGui::TreeNode("radialBlur")) {
		ImGui::Checkbox("OnOff", &isRadialBlur);

		if (isRadialBlur) {
			ImGui::DragFloat2("blurCenter", &blurCenter.x, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("blurWidth", &blurWidth, 0.001f, 0.0f, 0.1f);
			ImGui::DragInt("blurSamples", &blurSamples, 1, 1, 100);
		}

		ImGui::TreePop();
	}
	// ディスタンスフォグ
	if (ImGui::TreeNode("distanceFog")) {
		ImGui::Checkbox("OnOff", &isDistanceFog);

		if (isDistanceFog) {
			ImGui::ColorEdit3("fogColor", &distanceFogColor.x);
			ImGui::DragFloat("fogStart", &distanceStart, 0.1f, 0.0f, 100.0f);
			ImGui::DragFloat("fogEnd", &distanceEnd, 0.1f, 0.0f, 100.0f);
		}

		ImGui::TreePop();
	}
	// ハイトフォグ
	if (ImGui::TreeNode("heightFog")) {
		ImGui::Checkbox("OnOff", &isHeightFog);

		if (isHeightFog) {
			ImGui::ColorEdit3("heightFogColor", &heightFogColor.x);
			ImGui::DragFloat("heightFogTop", &heightFogTop, 0.1f, -100.0f, 100.0f);
			ImGui::DragFloat("heightFogBottom", &heightFogBottom, 0.1f, -100.0f, 100.0f);
			ImGui::DragFloat("heightFogDensity", &heightFogDensity, 0.01f, 0.0f, 10.0f);
		}

		ImGui::TreePop();
	}
	// DOF
	if (ImGui::TreeNode("DOF")) {
		ImGui::Checkbox("OnOff", &isDOF);

		if (isDOF) {
			ImGui::DragFloat("focusDistance", &focusDistance, 0.1f, 0.0f, 100.0f);
			ImGui::DragFloat("bokehRadius", &bokehRadius, 0.1f, 0.0f, 100.0f);
			ImGui::DragFloat("focusRange", &focusRange, 0.1f, 0.0f, 100.0f);
		}

		ImGui::TreePop();
	}
	// ブルーム
	if (ImGui::TreeNode("Bloom")) {
		ImGui::DragFloat("bloomThreshold", &bloomThreshold, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("bloomIntensity", &bloomIntensity, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("bloomRadius", &bloomBlurRadius, 0.01f, 0.0f, 10.0f);

		ImGui::TreePop();
	}
	// レンズフレア
	if (ImGui::TreeNode("LensFlare")) {
		ImGui::Checkbox("OnOff", &isLensFlare);

		if (isLensFlare) {
			ImGui::DragInt("lensFlareGhostCount", &lensFlareGhostCount, 1, 0,10);
			ImGui::DragFloat("lensFlareHaloWidth", &lensFlareHaloWidth, 0.01f, 0.0f, 10.0f);
			ImGui::Checkbox("isACES", &isACES);
			ImGui::DragFloat("caIntensity", &caIntensity, 0.001f, 0.0f, 10.0f);
		}
		ImGui::Text("%.3f", PostEffect::GetInstance()->GetLensFlareGhostDispersal());

		ImGui::TreePop();
	}
	// モーションブラー
	if (ImGui::TreeNode("MotionBlur")) {
		ImGui::Checkbox("OnOff", &isMotionBlur);

		if (isLensFlare) {
			ImGui::DragInt("motionBlurSamples", &motionBlurSamples, 1, 0, 20);
			ImGui::DragFloat("motionBlurScale", &motionBlurScale, 0.01f, 0.0f, 10.0f);
		}

		ImGui::TreePop();
	}
	// 色収差
	if (ImGui::TreeNode("CA")) {
		ImGui::Checkbox("OnOff", &isFullScreenCA);

		if (isFullScreenCA) {
			ImGui::DragFloat("fullScreenCAIntensity", &fullScreenCAIntensity, 0.001f, 0.0f, 1.0f);
		}

		ImGui::TreePop();
	}
	// スピードディストーション
	if (ImGui::TreeNode("SpeedDistortion")) {
		ImGui::Checkbox("OnOff", &isSpeedDistortion);
		if (isSpeedDistortion) {
			ImGui::DragFloat("speedDistortionStrength", &speedDistortionStrength, 0.01f, 0.0f, 10.0f);
		}
		ImGui::TreePop();
	}
	// 集中線
	if (ImGui::TreeNode("ConcentrationLines")) {
		ImGui::Checkbox("OnOff", &isConcentrationLines);
		if (isConcentrationLines) {
			ImGui::DragFloat("concentrationLineIntensity", &concentrationLineIntensity, 0.01f, 0.0f, 10.0f);
			ImGui::DragFloat2("concentrationLineCenter", &concentrationLineCenter.x, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("concentrationLineDensity", &concentrationLineDensity, 1.0f, 0.0f, 2000.0f);
			ImGui::DragFloat("concentrationLineLength", &concentrationLineLength, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("concentrationLineSpeed", &concentrationLineSpeed, 0.01f, 0.0f, 20.0f);
		}
		ImGui::TreePop();
	}
	// ピンチエフェクト
	if (ImGui::TreeNode("PinchEffect")) {
		ImGui::Checkbox("OnOff", &isPinchEffect);
		if (isPinchEffect) {
			ImGui::DragFloat("pinchStrength", &pinchStrength, 0.01f, 0.0f, 10.0f);
			ImGui::DragFloat2("pinchCenter", &pinchCenter.x, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("pinchRadius", &pinchRadius, 0.01f, 0.0f, 1.0f);
		}
		ImGui::TreePop();
	}

#pragma endregion

#pragma region レイマーチング
	// レイマーチング
	//ImGui::DragFloat("rayMarchingTime", &rayMarchingTime, 0.1f,0.0f,10.0f);
	ImGui::DragFloat3("rayMarchingSunDir", &rayMarchingSunDir.x, 0.01f,-1.0f,1.0f);
	ImGui::DragFloat("rayMarchingCloudCoverage", &rayMarchingCloudCoverage, 0.01f,-5.0f,10.0f);
	ImGui::DragFloat("rayMarchingCloudBottom", &rayMarchingCloudBottom, 10.0f,-5000.0f,5000.0f);
	ImGui::DragFloat("rayMarchingCloudTop", &rayMarchingCloudTop, 10.0f, -5000.0f, 5000.0f);
	ImGui::Checkbox("rayMarchingIsRialLight", &rayMarchingIsRialLight);
	ImGui::Checkbox("rayMarchingIsAnimeLight", &rayMarchingIsAnimeLight);
	ImGui::Checkbox("rayMarchingIsMotionBlur", &rayMarchingIsMotionBlur);
	ImGui::DragFloat("rayMarchingCloudOpacity", &rayMarchingCloudOpacity, 0.001f, 0.0f, 0.1f);
	ImGui::Checkbox("isStorm", &isStorm);
	ImGui::DragFloat("thunderFrequency", &thunderFrequency, 0.001f, 0.0f, 10.0f);
	ImGui::DragFloat("thunderBrightness", &thunderBrightness, 0.01f, 0.0f, 300.0f);


#pragma endregion

#endif

}

void TitleScene::Draw2D() {
	// 2Dオブジェクトの描画準備
	SpriteCommon::GetInstance()->SetCommonPipelineState();

	// スプライト描画
	title_->Draw();

}
void TitleScene::Draw3D() {
	// 3Dオブジェクトの描画準備
	ObjectCommon::GetInstance()->SetCommonDrawSetting();

	
	// 3Dオブジェクト描画
	for (int i = 0; i < 2; i++) {
		object[i]->Draw();
	}

	// 本型UIの描画準備
	//BookUiCommon::GetInstance()->SetCommonPipelineState();

	//book->Draw();

	//アニメーションモデル描画
	ObjectCommon::GetInstance()->SetSkinningCommonDrawSetting();

	// アニメーションモデルの描画
	for (auto& object : animationObjects) {
		if (object->IsSkeletal()) {
			object->Draw();
		}
	}

	// アウトライン描画準備
	//ObjectCommon::GetInstance()->SetOutlinePipelineState();
	//
	//// アウトライン描画
	//for (int i = 0; i < 2; i++) {
	//	object[i]->Draw();
	//}

}

void TitleScene::Finalize() {
	CameraManager::GetInstance()->RemoveCamera("main");
	animationObjects.clear();
}
