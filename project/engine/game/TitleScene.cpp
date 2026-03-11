#include "TitleScene.h"
#include "ObjectCommon.h"
#include "SpriteCommon.h"
#include "SceneManager.h"

void TitleScene::Initialize() {

	// レールカメラ初期化
	railCamera = std::make_unique <RailCamera>();
	railCamera->Initialize();

	// カメラマネージャ登録
	//CameraManager::GetInstance()->AddCamera("main", camera.get());
	//CameraManager::GetInstance()->SetActiveCamera("main");

	// スプライト
	sprite = std::make_unique <Sprite>();
	sprite->Initialize("Resource/monsterBall.png");

	// 3Dオブジェクト
	for (int i = 0; i < 2; i++) {
		object[i] = std::make_unique <Object>();
		object[i]->Initialize(railCamera->camera.get());
	}

	// Emitパーティクル発生
	particleEmitter = std::make_unique <ParticleEmitter>();
	particleEmitter->Initialize("group1", transformParticle, 5, 1.0f);
	particleEmitter->Emit();

	// 初期化済みの3Dオブジェクトにモデルを紐づける
	object[0]->SetModel("plane.obj");
	object[1]->SetModel("skydome.obj");

	// 音声再生
	SoundManager::GetInstance()->Play("bgm");

	// レイマーチング
	rayMarching = std::make_unique <RayMarching>();
	rayMarching->Initialize(railCamera->camera.get());

}

void TitleScene::Update() {
	// 入力取得
	auto input = Input::GetInstance();
	// カメラ更新
	//CameraManager::GetInstance()->Update();

	railCamera->Update();
	railCamera->EditorUpdate();

	// ENTERキーを押したら
	//if (input->TriggerKey(DIK_RETURN)) {
	//	// ゲームプレイシーン(次シーン)を生成
	//	SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
	//	// 音声再生
	//	SoundManager::GetInstance()->Stop("bgm");
	//}

	// 数字の０キーが押されていたら
	if (input->TriggerKey(DIK_0)) {
		OutputDebugStringA("Hit 0\n"); // 出力ウィンドウに「Hit ０」と表示
		// テクスチャ変更
		sprite->ChangeTexture("Resource/uvChecker.png");
		particleEmitter->SetActive("group2");

		// エフェクト有効化(色反転)
		PostEffect::GetInstance()->SetInversion(true);
	}

	// * 3Dオブジェクト* //
	for (int i = 0; i < 2; i++) {
		object[i]->Update();
	}

	// パーティクルエミッタ更新
	particleEmitter->Update();


	// *スプライト* //
	// sprite更新
	sprite->Update();

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
	PostEffect::GetInstance()->HightFogUpdate(railCamera->camera.get());
	// DOF
	PostEffect::GetInstance()->SetDOF(isDOF);
	PostEffect::GetInstance()->SetFocusDistance(focusDistance);
	PostEffect::GetInstance()->SetBokehRadius(bokehRadius);
	PostEffect::GetInstance()->SetFocusRange(focusRange);

#pragma endregion

#pragma region レイマーチング
	// レイマーチング
	rayMarching->Update(railCamera->camera.get());
	//rayMarching->SetTime(rayMarchingTime);
	rayMarching->SetSunDir(rayMarchingSunDir);
	rayMarching->SetDensity(rayMarchingDensity);
	rayMarching->SetCloudTop(rayMarchingCloudBottom);
	rayMarching->SetCloudBottom(rayMarchingCloudTop);
	rayMarching->SetRialLight(rayMarchingIsRialLight);
	rayMarching->SetAnimeLight(rayMarchingIsAnimeLight);

#pragma endregion

#ifdef USE_IMGUI
	// ImGui


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

#pragma endregion

#pragma region レイマーチング
	// レイマーチング
	//ImGui::DragFloat("rayMarchingTime", &rayMarchingTime, 0.1f,0.0f,10.0f);
	ImGui::DragFloat3("rayMarchingSunDir", &rayMarchingSunDir.x, 0.1f,-50.0f,50.0f);
	ImGui::DragFloat("rayMarchingDensity", &rayMarchingDensity, 0.01f,-5.0f,1.0f);
	ImGui::DragFloat("rayMarchingCloudBottom", &rayMarchingCloudBottom, 10.0f,-5000.0f,5000.0f);
	ImGui::DragFloat("rayMarchingCloudTop", &rayMarchingCloudTop, 10.0f, -5000.0f, 5000.0f);
	ImGui::Checkbox("rayMarchingIsRialLight", &rayMarchingIsRialLight);
	ImGui::Checkbox("rayMarchingIsAnimeLight", &rayMarchingIsAnimeLight);

#pragma endregion

#endif

}

void TitleScene::Draw2D() {
	// 2Dオブジェクトの描画準備
	SpriteCommon::GetInstance()->SetCommonPipelineState();

	// スプライト描画
	//sprite->Draw();
}
void TitleScene::Draw3D() {
	// 3Dオブジェクトの描画準備
	ObjectCommon::GetInstance()->SetCommonPipelineState();

	// レールカメラエディター
	railCamera->EditorDraw();

	// 3Dオブジェクト描画
	for (int i = 0; i < 2; i++) {
		object[i]->Draw();
	}

	// パーティクル描画
	ParticleManager::GetInstance()->Draw();

	// アウトライン描画準備
	ObjectCommon::GetInstance()->SetOutlinePipelineState();

	// アウトライン描画
	for (int i = 0; i < 2; i++) {
		object[i]->Draw();
	}

	// レイマーチング
	rayMarching->Draw();
}

void TitleScene::Finalize() {
	CameraManager::GetInstance()->RemoveCamera("main");
}
