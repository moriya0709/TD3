#include "resultScene.h"
#include "SceneManager.h"
#include <SpriteCommon.h>
#include <ObjectCommon.h>

void ResultScene::Initialize()
{
	//初期化
	targetScore_ = 0;
	for (int i = 0; i < 5; i++)
	{
		actualDigits_[i] = 0;
		displayNumbers_[i] = 0;
	}


	ScoreManager scoreManager;
	//過去の全データを読み込む
	history_ = scoreManager.LoadHistory();

	if (!history_.empty())
	{
		//最新のスコアを取得
		targetScore_ = history_.back().score;

		//スコアを5桁に分割
		actualDigits_[0] = (targetScore_ / 10000) % 10;
		actualDigits_[1] = (targetScore_ / 1000) % 10;
		actualDigits_[2] = (targetScore_ / 100) % 10;
		actualDigits_[3] = (targetScore_ / 10) % 10;
		actualDigits_[4] = targetScore_ % 10;
	}

	//スコアシャッフル
	currentDigitIndex_ = 0;
	countTimer_ = 0.0f;
	isScoreStartTime_ = true;
	isCanPress_ = false;

	// カメラ初期化
	camera = std::make_unique<Camera>();
	camera->SetRotate({ cameraTransform.rotate });
	camera->SetTranslate({ cameraTransform.translate });

	// カメラマネージャ登録
	CameraManager::GetInstance()->AddCamera("main", camera.get());
	CameraManager::GetInstance()->SetActiveCamera("main");

	for (int num = 0; num < 10; num++)
	{
		for (int digit = 0; digit < 5; digit++)
		{
			numberSprites_[num][digit] = std::make_unique<Sprite>();
			//数字
			std::string filePath = "Resource/number/" + std::to_string(num) + ".png";
			numberSprites_[num][digit]->Initialize(filePath);
		}
	}



	sprite = std::make_unique <Sprite>();
	sprite->Initialize("Resource/monsterBall.png");

	// 3Dオブジェクト
	for (int i = 0; i < 2; i++) {
		object[i] = std::make_unique <Object>();
		object[i]->Initialize(camera.get());
	}

	// Emitパーティクル発生
	particleEmitter = std::make_unique <ParticleEmitter>();
	particleEmitter->Initialize("group1", transformParticle, 5, 1.0f);
	particleEmitter->Emit();
	//particleEmitter->LoadParticle("Resource/particle/fire.csv");

	// 初期化済みの3Dオブジェクトにモデルを紐づける
	//object[0]->SetModel("emission.obj");
	//object[1]->SetModel("skydome.obj");

}

void ResultScene::Update()
{
	// 入力取得
	auto input = Input::GetInstance();
	// カメラ更新
	CameraManager::GetInstance()->Update();

	//スペースキーでお急ぎ用スコア表示
	if (isScoreStartTime_ && !isCanPress_)
	{
		if (input->TriggerKey(DIK_SPACE))
		{
			currentDigitIndex_ = 5;//スコア全部強制確定
		}

		//まだ出てない桁があればタイマーを進める
		if (currentDigitIndex_ < 5)
		{
			countTimer_ += 1.0f / 60.0f;

			// 全5桁の表示用数字を計算
			for (int i = 0; i < 5; i++)
			{
				// currentDigitIndex_ が 0 の時：全員シャッフル
				// 1 の時：i=4(右端)が確定
				if (i >= (5 - currentDigitIndex_)) {
					displayNumbers_[i] = actualDigits_[i]; // 確定(スコアを上書き)
				} else {
					displayNumbers_[i] = rand() % 10; // シャッフル中
				}
			}

			//2秒経過したら、次の桁のシャッフル開始
			if (countTimer_ >= kMaxCount_)
			{
				countTimer_ = 0.0f;//タイマーリセット
				currentDigitIndex_++;//確定する1桁を1つ進める
			}
		} else {//5桁全て確定したらステージセレクト(Enterキー押せるよう)にする
			for (int i = 0; i < 5; i++)
			{
				displayNumbers_[i] = actualDigits_[i];//一応全部本物のスコアに変える
			}
			isCanPress_ = true;
		}
	}
		//ステージセレクトへ
	if (isCanPress_)
	{
		//Enterキーで
		if (input->TriggerKey(DIK_RETURN)) {
			// ゲームプレイシーン(次シーン)を生成
			SceneManager::GetInstance()->ChangeScene("GAMESELECT");
		}
	}

	// * 3Dオブジェクト* //
	for (int i = 0; i < 2; i++) {
		object[i]->Update();
	}

	// パーティクルエミッタ更新
	particleEmitter->Editor();
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
	PostEffect::GetInstance()->Update(camera.get());

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
			ImGui::DragInt("lensFlareGhostCount", &lensFlareGhostCount, 1, 0, 10);
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

		if (isLensFlare) {
			ImGui::DragFloat("fullScreenCAIntensity", &fullScreenCAIntensity, 0.001f, 0.0f, 1.0f);
		}

		ImGui::TreePop();
	}

#pragma endregion

#pragma region レイマーチング
	// レイマーチング
	//ImGui::DragFloat("rayMarchingTime", &rayMarchingTime, 0.1f,0.0f,10.0f);
	ImGui::DragFloat3("rayMarchingSunDir", &rayMarchingSunDir.x, 0.01f, -1.0f, 1.0f);
	ImGui::DragFloat("rayMarchingCloudCoverage", &rayMarchingCloudCoverage, 0.01f, -5.0f, 10.0f);
	ImGui::DragFloat("rayMarchingCloudBottom", &rayMarchingCloudBottom, 10.0f, -5000.0f, 5000.0f);
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

void ResultScene::Draw2D()
{
	// 2Dオブジェクトの描画準備
	SpriteCommon::GetInstance()->SetCommonPipelineState();

	if (isScoreStartTime_)
	{
		for (int i = 0; i < 5; i++)
		{
			Vector2 drawPosition = { 850.0f + (i * 60.0f),500.0f };

			//その桁で表示する数字
			int number = displayNumbers_[i];

			numberSprites_[number][i]->SetPosition(drawPosition);
			numberSprites_[number][i]->Update();
			numberSprites_[number][i]->Draw();
		}
	}
}

void ResultScene::Draw3D()
{

	// アウトライン描画準備
	ObjectCommon::GetInstance()->SetOutlinePipelineState();
}

void ResultScene::Finalize()
{

}
