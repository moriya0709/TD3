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

	///
	///タイトル
	/// 

	struct TitleData
	{
		std::string filePath;
		Vector2 pos;
		float rot;
		//Vector2 anchor;//文字がある場所の比率
	};

	//初期化データ配列
	std::vector<TitleData> initDatas = {
	{ "Resource/title/titleF.png",           { 950.0f, 300.0f },0.0f,},  // フ
	{ "Resource/title/titleR.png",           { 950.0f, 300.0f },0.0f, },  // ル
	{ "Resource/title/title-.png",           { 950.0f, 300.0f },0.0f, },  // ー
	{ "Resource/title/titleT.png",           { 950.0f, 300.0f },0.0f, },  // ツ
	{ "Resource/title/titleKamiHikouki.png", { 950.0f, 300.0f },0.0f, },  // 紙飛行機
	{ "Resource/title/titleCloud.png",       { 950.0f, 300.0f },0.0f, },  // タの雲
	{ "Resource/title/titleB.png",           { 950.0f, 300.0f },0.0f, },  // バスタズ
	{ "Resource/title/titleEgg.png",         { 950.0f, 300.0f },0.0f, },  // バの濁点
	{ "Resource/title/titleHouki.png",       { 950.0f, 300.0f },0.0f, },  // ー
	{ "Resource/title/titleKusege.png",      { 950.0f, 300.0f },0.0f, },  // ズの頭
	{ "Resource/title/titleFoot.png",        { 950.0f, 300.0f },0.0f, },  // ズの足
	{ "Resource/title/titleNasu.png",        { 950.0f, 300.0f },0.0f, }   // ズの濁点
	};

	//リストのクリアと確保
	titleParts_.clear();
	titleParts_.reserve(initDatas.size());
	int index = 0;//ループで一気に生成してリストに格納

	//ループと一気に生成してリストに格納
	for (const auto& data : initDatas)
	{
		TitlePart part;
		part.sprite = std::make_unique<Sprite>();
		part.sprite->Initialize(data.filePath);
		part.sprite->SetPosition(data.pos);
		part.sprite->SetRotation(data.rot);
		//part.sprite->SetAnchorPoint(data.anchor);

		//独自のモーション用変数
		part.position = data.pos;
		part.rotation = data.rot;
		part.velocity = { 0.0f,0.0f };
		part.timer = 0.0f;
		part.id = static_cast<TitleMove>(index);//番号でtitleのパーツを記憶

		//リストに追加
		titleParts_.push_back(std::move(part));
		index++;
	}

	nextM_ = std::make_unique<Sprite>();
	nextM_->Initialize("Resource/nextM.png"); // 進める
	nextM_->SetPosition({ 950.0f, 900.0f });

	nextC_ = std::make_unique<Sprite>();
	nextC_->Initialize("Resource/nextC.png"); // 進める
	nextC_->SetPosition({ 950.0f, 900.0f });

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

/*	//アニメーションデータの読み込み(モデル自体はGame.cppに入れること)
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

	animationObjects.push_back(std::move(walkAnim));//アニメーションモデル専用のリストに入れる*/

	
	// 初期化済みの3Dオブジェクトにモデルを紐づける
	object[0]->SetModel("emission.obj");
	object[1]->SetModel("skydome.obj");

	// 音声再生
	SoundManager::GetInstance()->Play("title.mp3",true, bgmVolume_);
}

void TitleScene::Update() {
	// 入力取得
	auto input = Input::GetInstance();
	// カメラ更新
	CameraManager::GetInstance()->Update();

	// カメラ更新
	cameraTransform.translate.x += 0.05f;
	cameraTransform.translate.z += 0.05f;
	camera->SetTranslate(cameraTransform.translate);

	// エフェクトの強さ減少
	if (isTransition) {
		intensity = (std::max)(0.0f, intensity - 1.0f / 30.0f);

		// BGMのフェードアウト処理
		bgmVolume_ = (std::max)(0.0f, bgmVolume_ - 1.0f / 30.0f); // 音量も同じ速度で減少
		SoundManager::GetInstance()->SetVolume("title.mp3", bgmVolume_);

		if(intensity <= 0.0f){
			// シーン切り替え処理
			SoundManager::GetInstance()->Stop("title.mp3");
			SceneManager::GetInstance()->ChangeScene("GAMESELECT");
		}

	} else {
		if (intensity <= 1.0f)
			intensity += 1.0f / 30.0f; // 1秒かけて明るくする
	}

	///
	/// タイトル処理
	///

	//パーツごとにタイマーを進めて動かす
	for (auto& part : titleParts_)
	{
		//各パーツタイマーの毎フレーム加算
		part.timer += 1.0f / 60.0f;

		//10秒経ったら0秒にループ
		if (part.timer >= 10.0f)
		{
			part.timer = 0.0f;
		}

		///
		///パーツを動かす
		/// 
		
		switch (part.id)
		{
		case titleF://フ
			break;
		case titleR://ル

			break;

		case titleI://フルーツのー
			
			//0秒から6秒で小回転する
			if (part.timer >= 0.0f && part.timer < 6.0f) {

				float rate = part.timer / 6.0f; // 0.0 ～ 1.0 の進捗率

				// 6秒かけて1往復半
				float swing = std::sin(rate * 3.141592f * 3.0f);

				// 4.0fという数字を処理側に固定しておくことで、ちょうど2往復して綺麗に正面に
				float envelope = std::sin(rate * 3.141592f);

				// スピードを落とし滑らかに着地
				part.rotation = swing * envelope * 0.35f;
			}
			else {
				part.rotation = 0.0f;
			}

			break;

		case titleT://ツ
			break;

		case titleKami://紙飛行機
			
			break;
		case titleCloud://タの雲部分

			break;

		case titleB://バスタズ

			break;

		case titleEgg://バの濁点

			break;

		case titleHouki://バスターズのー

			break;

		case titleKusege://ズの頭

			break;

		case titleFoot://ズの足

			break;

		case titleNasu://ズの濁点

			break;
		}


		//計算した座標を全てのスプライトに反映
		part.sprite->SetPosition(part.position);
		part.sprite->SetRotation(part.rotation);
		part.sprite->Update();
	}


	// SPACEキーを押したら
	if (input->TriggerKey(DIK_SPACE) || input->IsPadButtonPressed(0, 1)) {
		// ゲームプレイシーン(次シーン)を生成
		//SceneManager::GetInstance()->ChangeScene("GAMESELECT");
		isTransition = true;
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

	spaceTimer_ += 0.05f;
	float sinTimer = std::sin(spaceTimer_);//-1.0f～1.0fの範囲
	if (Input::GetInstance()->GetCurrentDevice() == InputDevice::Gamepad)
	{
		nextC_->SetColor(Vector4(1.0f, 1.0f, 1.0f, ((sinTimer + 1.0f) / 2.0f)));//透明演出
		nextC_->Update();
	}
	else {
		nextM_->SetColor(Vector4(1.0f, 1.0f, 1.0f, ((sinTimer + 1.0f) / 2.0f)));//透明演出
		nextM_->Update();
	}

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
	// エフェクトの強さ
	PostEffect::GetInstance()->SetIntensity(intensity);

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
	for (auto& part : titleParts_)
	{
		//各スプライトが持つ
		part.sprite->Draw();
	}
	if (Input::GetInstance()->GetCurrentDevice() == InputDevice::Gamepad)
	{
		nextC_->Draw();
	}
	else {
		nextM_->Draw();
	}
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
}
