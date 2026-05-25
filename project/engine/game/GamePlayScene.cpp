#include "GamePlayScene.h"
#include "BananaCameraController.h"
#include "GrapeCameraController.h"
#include "Model.h"
#include "ObjectCommon.h"
#include "SceneManager.h"
#include "ScoreManager.h"
#include "SpriteCommon.h"
#include "StageCameraController.h"

void GamePlayScene::Initialize() {
	// カメラ初期化
	camera = std::make_unique<Camera>();
	camera->SetRotate({cameraTransform.rotate});
	camera->SetTranslate({cameraTransform.translate});

	// カメラマネージャ登録
	CameraManager::GetInstance()->AddCamera("main", camera.get());
	CameraManager::GetInstance()->SetActiveCamera("main");

	cameraController_ = std::make_unique<StageCameraController>();
	cameraController_->Initialize(camera.get());

	player_ = std::make_unique<Player>();
	player_->Initialize(camera.get(), style_);
	maxHP_ = player_->GetHP();
	enemy_ = std::make_unique<EnemyManager>();
	if (currentStage_ == 2) {
		isBossBattle_ = true;
		bossPopFlag = 2;

	} else if (currentStage_ == 5) {
		isBossBattle_ = true;
		bossPopFlag = 3;
	}

	cameraController_->SetCurrentStage(currentStage_);
	cameraController_->StartReplay();
	kMaxTime_ = cameraController_->GetTotalDuration();
	enemy_->Initialize(player_.get(), camera.get(), cameraController_.get());

	///
	/// アニメーションモデル
	///

	// スケルトン
	Model* model = ModelManager::GetInstance()->FindModel("simpleSkin.gltf"); // スケルトンアクセス権
	skeleton_ = model->CreateSkeleton(model->GetModelData().rootNode);        // 動く仕組み

	// アニメーションデータの読み込み(モデル自体はGame.cppに入れること)
	simpleAnimation_ = Model::LoadAnimationFile("./Resource", "simpleSkin.gltf"); // スケルトン
	walkAnimation_ = Model::LoadAnimationFile("./Resource", "walk.gltf");

	///
	///
	///

	// スプライト
	pause_ = std::make_unique<Sprite>();
	pause_->Initialize("Resource/pause.png"); // ポーズ
	pause_->SetPosition({1850.0f, 50.0f});

	resume_ = std::make_unique<Sprite>();
	resume_->Initialize("Resource/resume.png"); // 続ける
	resume_->SetPosition({960.0f, 216.0f});

	resumeEasing.size = {0.0f, 0.0f};
	resumeEasing.startSizeV2 = {0.0f, 0.0f};
	resumeEasing.endSizeV2 = {400.0f, 400.0f};
	resumeEasing.sizeTime = 0.0f;
	resumeEasing.sizeEasedT = 0.0f;

	retry_ = std::make_unique<Sprite>();
	retry_->Initialize("Resource/retry.png"); // リトライ
	retry_->SetPosition({860.0f, 432.0f});
	retryEasing.size = {0.0f, 0.0f};
	retryEasing.startSizeV2 = {0.0f, 0.0f};
	retryEasing.endSizeV2 = {300.0f, 300.0f};
	retryEasing.sizeTime = 0.0f;
	retryEasing.sizeEasedT = 0.0f;

	select_ = std::make_unique<Sprite>();
	select_->Initialize("Resource/select.png"); // セレクトへ
	select_->SetPosition({1060.0f, 648.0f});
	selectEasing.size = {0.0f, 0.0f};
	selectEasing.startSizeV2 = {0.0f, 0.0f};
	selectEasing.endSizeV2 = {300.0f, 300.0f};
	selectEasing.sizeTime = 0.0f;
	selectEasing.sizeEasedT = 0.0f;

	// playerHPバーのUI部分(外枠)
	playerHpUI_ = std::make_unique<Sprite>();
	playerHpUI_->Initialize("Resource/UI/playerHp.png");
	playerHpUI_->SetAnchorPoint({0.0f, 0.0f});
	playerHpUI_->SetPosition({8.0f, 10.0f});
	playerHpUI_->SetSize({240.0f, 50.0f});
	// playerHPのHPゲージ部分
	playerHPGauge_ = std::make_unique<Sprite>();
	playerHPGauge_->Initialize("Resource/white.png");
	playerHPGauge_->SetAnchorPoint({0.0f, 0.0f}); // サイズ調整
	playerHPGauge_->SetPosition({39.0f, 22.0f});  // UIの透過部分に合うように右に
	// playerHPのゲージが減った時の空部分
	playerHPEmpty_ = std::make_unique<Sprite>();
	playerHPEmpty_->Initialize("Resource/white.png");
	playerHPEmpty_->SetAnchorPoint({0.0f, 0.0f}); // サイズ調整
	playerHPEmpty_->SetPosition({39.0f, 22.0f});  // UIの透過部分に合うように

	pauseBg_ = std::make_unique<Sprite>();
	pauseBg_->Initialize("Resource/pauseBg.png"); // ポーズ背景
	pauseBg_->SetPosition({960.0f, 540.0f});
	pauseBg_->SetSize({1920.0f, 1080.0f});

	// 特殊攻撃のエフェクト
	specialAttackEffect = std::make_unique<ParticleEmitter>();
	specialAttackEffect->Initialize("SpecialAttack", transformParticle, 100, 0.1f);
	specialAttackEffect->SetActive("SpecialAttack");
	specialAttackEffect->LoadParticle("Resource/particle/special_1.csv");

	// ヒットエフェクト
	hitEffect = std::make_unique<ParticleEmitter>();
	hitEffect->Initialize("HitEffect", transformParticle, 50, 0.1f);
	hitEffect->SetActive("HitEffect");
	hitEffect->LoadParticle("Resource/particle/hit_1.csv");

	// ゲームオーバー

	gameOver_ = std::make_unique<Sprite>();
	gameOver_->Initialize("Resource/gameOverUi/gameOver.png");
	gameOver_->SetPosition({ 960.0f, 560.0f });
	gameOver_->SetSize({ 1000.0f, 1000.0f });
	for (int i = 0; i < 2; i++)
		gameOverUi_[i] = std::make_unique<Sprite>();
	gameOverUi_[0]->Initialize("Resource/gameOverUi/restartUi.png");
	gameOverUi_[1]->Initialize("Resource/gameOverUi/selectUi.png");
	gameOverUi_[0]->SetPosition({ 700.0f, 800.0f });
	gameOverUi_[1]->SetPosition({ 1220.0f, 800.0f });
	gameOverUi_[0]->SetSize({300.0f, 300.0f});
	gameOverUi_[1]->SetSize({300.0f, 300.0f});
	gameOverUi_[0]->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
	gameOverUi_[1]->SetColor({ 0.1f, 0.1f, 0.1f, 0.0f });

	for (int i = 0; i < kGameOverUi_; i++) {
		gameOverEasing_[i].colorTime = 0.0f;
		gameOverEasing_[i].colorEasedT = 0.0f;
	}
	gameOverEasing_[0].startColor = { 1.0f, 1.0f, 1.0f, 0.0f };
	gameOverEasing_[1].startColor = { 1.0f, 1.0f, 1.0f, 0.0f };
	gameOverEasing_[2].startColor = { 0.1f, 0.1f, 0.1f, 0.0f };
	gameOverEasing_[0].endColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	gameOverEasing_[1].endColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	gameOverEasing_[2].endColor = { 0.1f, 0.1f, 0.1f, 1.0f };

	// イージング
	easing = std::make_unique<Easing>();
	easing->Initialize();

}

void GamePlayScene::Update() {

	if (!isPause_) {
		// セレクトからシーン切り替えした時のエフェクト
		SceneChangedEffect();

		// カメラ更新
		if (!isGameOver)
			cameraController_->Update();

		// アニメーションするモデル更新処理
		for (auto& object : animationObjects) {
			object->Update();
		}
		// プレイヤー更新
		player_->Update(enemy_->GetEnemies(), cameraController_->GetSpeed());

		// 敵更新
		enemy_->SetcurrentTimer_(cameraController_->GetElapsedTime());
		enemy_->Update();

		//敵から回収したスコアを自分のスコアに加算する
		this->score_ += enemy_->GiveScore();

		// 当たり判定
		ChekeAllCollision();

		// ポーズ画面へ
		if (Input::GetInstance()->TriggerKey(DIK_ESCAPE)) {
			isPause_ = true;
			currentPause_ = Pause::kResume;
		}

	} else { // ポーズ画面
		PauseSelect();
	}

	if (isPause_ || isFinished_)
		return;

	// hpが0以下にならないようにclamp
	float hpRate = std::clamp((float)player_->GetHP() / (float)maxHP_, 0.0f, 1.0f);
	float maxBarWidth = 194.0f; // 枠に収まる最大幅
	// ゲージサイズを設定{横幅, 縦幅}
	playerHPGauge_->SetSize({maxBarWidth * hpRate, 30.0f});
	playerHPEmpty_->SetSize({maxBarWidth, 30.0f});

	// ゲージの色を変える
	playerHPGauge_->SetColor(Vector4(0.0f, 1.0f, 1.0f, 1.0f)); // 水色(ゲージ部分)
	playerHPEmpty_->SetColor(Vector4(0.2f, 0.2f, 0.2f, 1.0f)); // 暗いグレー(空部分)

	// クリア条件の分岐
	if (isBossBattle_) {

		if (cameraController_->GetElapsedTime() >= kMaxTime_) {
			if (bossPopFlag == 2) {
				enemy_->SetEnemyclear();
				cameraController_ = std::make_unique<GrapeCameraController>();
				cameraController_->Initialize(camera.get());
				enemy_->Initialize(player_.get(), camera.get(), cameraController_.get());
				bossPopFlag = 4;
			} else if (bossPopFlag == 3) {
				enemy_->SetEnemyclear();
				cameraController_ = std::make_unique<BananaCameraController>();
				cameraController_->Initialize(camera.get());
				cameraController_->SetTargetPosition({0, 0, 60});
				enemy_->Initialize(player_.get(), camera.get(), cameraController_.get());
				bossPopFlag = 6;
			}
		}
		// ボスがいる場合はフラグを5にする
		if (enemy_->GetGBoss().size() > 0 && bossPopFlag == 4) {
			bossPopFlag = 5;
		}

		if (enemy_->GetGBoss().empty() && bossPopFlag == 5) { // 葡萄のボスがいなくなったら
			StageClear();
		}
		if (enemy_->GetBBoss().size() > 0 && bossPopFlag == 6) { // バナナのボスがいなくなったら
			bossPopFlag = 7;
		}
		if (enemy_->GetBBoss().empty() && bossPopFlag == 7) {
			StageClear();
		}

		// ボス倒したらクリア
	} else { // 制限時間来たらリザルトへ
		if (cameraController_->GetElapsedTime() >= kMaxTime_) {
			StageClear();
		}
	}
	// デス演出
	if (player_->GetHP() <= 0) {
		if (deathEffectTimer_ == 3.0f) {
			player_->StartDeathAnimation(); // デス演出開始
			isGameOver = true;
		}
		deathEffectTimer_ = (std::max)(0.0f, deathEffectTimer_ - 1.0f / 60.0f);

		if (deathEffectTimer_ <= 2.0f) {
			for (int i = 0; i < kGameOverUi_; i++) {
				if (gameOverEasing_[i].colorTime < 1.0f) {
					easing->Color(gameOverEasing_[i], 0.01f, 0);
				} else {
					if (Input::GetInstance()->TriggerKey(DIK_A)) {
						gameOverUi_[0]->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
						gameOverUi_[1]->SetColor({ 0.1f, 0.1f, 0.1f, 1.0f });

						currentGameOverUI_ = Pause::kRetry;

					}
					if (Input::GetInstance()->TriggerKey(DIK_D)) {
						gameOverUi_[0]->SetColor({ 0.1f, 0.1f, 0.1f, 1.0f });
						gameOverUi_[1]->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

						currentGameOverUI_ = Pause::kSelect;

					}
				}
			}

			if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
				if (currentGameOverUI_ == Pause::kSelect) {
					// セレクトシーンを生成
					SceneManager::GetInstance()->ChangeScene("GAMESELECT");

				} else if (currentGameOverUI_ == Pause::kRetry) {
					// ゲームプレイシーンを生成
					SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
				}
			}
		}

		// 色更新
		if (gameOverEasing_[0].colorTime < 1.0f) {
			gameOver_->SetColor(gameOverEasing_[0].color);
			gameOverUi_[0]->SetColor(gameOverEasing_[1].color);
			gameOverUi_[1]->SetColor(gameOverEasing_[2].color);
		}

		// 更新
		gameOver_->Update();
		gameOverUi_[0]->Update();
		gameOverUi_[1]->Update();

	}
	// スプライト更新
	pause_->Update();
	playerHpUI_->Update();
	playerHPEmpty_->Update();
	playerHPGauge_->Update();
	LithingEffect();
	UpdateImGui();

	if (!animationObjects.empty()) {
		Object* animationObject = animationObjects[0].get(); // アニメーションモデルを取得

		// アニメーションするモデル更新処理
		if (animationObject->IsSkeletal()) {
			Vector3 scale = animationObject->GetScale();
			Vector3 rotate = animationObject->GetRotate();
			Vector3 translate = animationObject->GetTranslate();

			// アニメーションモデルのワールド行列を作る
			Matrix4x4 animationWorldMatrix = MakeAffineMatrix(scale, rotate, translate);
		}
	}
	// イージング更新
	easing->Update();
	easing->Draw();
}

void GamePlayScene::Draw2D() {
	// 2Dオブジェクトの描画準備
	SpriteCommon::GetInstance()->SetCommonPipelineState();

	player_->Draw2D();

	pause_->Draw(); // ポーズ

	playerHpUI_->Draw();
	playerHPEmpty_->Draw();
	playerHPGauge_->Draw();

	if (player_->GetHP() <= 0) {
		gameOver_->Draw();
		for (int i = 0; i < 2; i++) {
			gameOverUi_[i]->Draw();
		}
	}

	if (isPause_) {
		pauseBg_->Draw(); // ポーズ背景
		resume_->Draw();  // ポーズ//続ける
		retry_->Draw();   // リトライ
		select_->Draw();  // セレクトへ
	}
}

void GamePlayScene::Draw3D() {
	// 3Dオブジェクトの描画準備
	ObjectCommon::GetInstance()->SetCommonDrawSetting();
	// 3Dオブジェクト描画
	player_->Draw3D();

	enemy_->Draw3D();

	cameraController_->EditorDraw();

	// アニメーションモデル描画
	ObjectCommon::GetInstance()->SetSkinningCommonDrawSetting();

	// アニメーションモデルの描画
	for (auto& object : animationObjects) {
		if (object->IsSkeletal()) {
			object->Draw();
		}
	}

	// パーティクル描画
	// ParticleManager::GetInstance()->Draw();

	// アウトライン描画準備
	ObjectCommon::GetInstance()->SetOutlinePipelineState();

	// player_->Draw3D();

	// アウトライン描画
	// object->Draw();
}

void GamePlayScene::Finalize() {
	CameraManager::GetInstance()->RemoveCamera("main");
	animationObjects.clear();
}

void GamePlayScene::SetPlayerStyle(int style) { style_ = static_cast<Style>(style); }

void GamePlayScene::SetCurrentStage(int currentStage) { currentStage_ = currentStage; }

void GamePlayScene::ChekeAllCollision() {
	const std::list<std::shared_ptr<Enemy>>& enemies = enemy_->GetEnemies();
	const std::list<std::shared_ptr<grapesBoss>>& Boss = enemy_->GetGBoss();
	const std::list<std::shared_ptr<banana>>& BBoss = enemy_->GetBBoss();
	CheckCollisionPlayerEnemy(player_.get(), enemies);
	CheckCollisionPlayerEnemyBullet(player_.get(), enemies);
	CheckCollisionPlayerBulletEnemy(player_.get(), enemies, hitEffect);
	CheckCollisionPlayerBulletBossEnemy(player_.get(), Boss, hitEffect);
	CheckCollisionPlayerBossEnemy(player_.get(), Boss);
	CheckCollisionPlayerBossEnemyBullet(player_.get(), Boss);
	CheckCollisionPlayerBulletBananaBoss(player_.get(), BBoss, hitEffect);
	CheckCollisionPlayerBananaBoss(player_.get(), BBoss);
	CheckCollisionPlayerBananaBossBullet(player_.get(), BBoss);

	if (player_->GetIsSpecialAttack() && specialAttackTimer <= 0) {
		CheckCollisionSpecialAtackEnemy(enemies);
		specialAttackTimer = 60; // 特殊攻撃のエフェクト時間（例: 60フレーム）

		// エフェクト初期化
		isInversion = true;          // 反転エフェクトa
		isGrayscale = true;          // グレースケールエフェクト
		isTwoColor = true;           // 2色エフェクト
		isConcentrationLines = true; // 集中線エフェクト
		isInversion = true;
	}
	if (specialAttackTimer > 0) {
		specialAttackTimer--;

		// 毎フレーム色反転
		if (specialAttackTimer > 50) {
			if (specialAttackTimer % 5 == 1) {
				isInversion = true;
			} else {
				isInversion = false;
			}
		}
		// 最初の10フレームのみエフェクトをかける
		if (specialAttackTimer == 50) {
			isInversion = false;          // 反転エフェクト
			isGrayscale = false;          // グレースケールエフェクト
			isTwoColor = false;           // 2色エフェクト
			isConcentrationLines = false; // 集中線エフェクト
		}

		// パーティクルの更新
		specialAttackEffect->SetTranslate(player_->GetPosition()); // プレイヤーの位置にエフェクトを移動
		specialAttackEffect->Update();

		if (specialAttackTimer <= 0) {
			player_->SetIsSpecialAttack(false); // 特殊攻撃の当たり判定は1フレームだけ

			specialAttackTimer = 0; // タイマーリセット
		}
	}
}

// ポーズ選択
void GamePlayScene::PauseSelect() {
	if (resumeEasing.sizeTime >= 1.0f && retryEasing.sizeTime >= 1.0f && selectEasing.sizeTime >= 1.0f)
		if (Input::GetInstance()->TriggerKey(DIK_ESCAPE) || Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			isPauseEasing_ = true;
			easingType_ = Start;

			resumeEasing.startSizeV2 = resumeEasing.size;
			resumeEasing.endSizeV2 = {0.0f, 0.0f};
			resumeEasing.sizeTime = 0.0f;
			resumeEasing.sizeEasedT = 0.0f;

			retryEasing.startSizeV2 = retryEasing.size;
			retryEasing.endSizeV2 = {0.0f, 0.0f};
			retryEasing.sizeTime = 0.0f;
			retryEasing.sizeEasedT = 0.0f;

			selectEasing.startSizeV2 = selectEasing.size;
			selectEasing.endSizeV2 = {0.0f, 0.0f};
			selectEasing.sizeTime = 0.0f;
			selectEasing.sizeEasedT = 0.0f;
		}

	if (isPauseEasing_) {
		if (selectEasing.sizeTime >= 1.0f) {
			isPause_ = false;
			isPauseEasing_ = false;

			resumeEasing.startSizeV2 = resumeEasing.size;
			resumeEasing.endSizeV2 = {400.0f, 400.0f};
			resumeEasing.sizeTime = 0.0f;
			resumeEasing.sizeEasedT = 0.0f;

			retryEasing.startSizeV2 = retryEasing.size;
			retryEasing.endSizeV2 = {300.0f, 300.0f};
			retryEasing.sizeTime = 0.0f;
			retryEasing.sizeEasedT = 0.0f;

			selectEasing.startSizeV2 = selectEasing.size;
			selectEasing.endSizeV2 = {300.0f, 300.0f};
			selectEasing.sizeTime = 0.0f;
			selectEasing.sizeEasedT = 0.0f;
		}
	}

	switch (currentPause_) {
	case Pause::kResume:
		if (resumeEasing.sizeTime >= 1.0f) {
			if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
				isPauseEasing_ = true;
				resumeEasing.startSizeV2 = resumeEasing.size;
				resumeEasing.endSizeV2 = {0.0f, 0.0f};
				resumeEasing.sizeTime = 0.0f;
				resumeEasing.sizeEasedT = 0.0f;

				retryEasing.startSizeV2 = retryEasing.size;
				retryEasing.endSizeV2 = {0.0f, 0.0f};
				retryEasing.sizeTime = 0.0f;
				retryEasing.sizeEasedT = 0.0f;

				selectEasing.startSizeV2 = selectEasing.size;
				selectEasing.endSizeV2 = {0.0f, 0.0f};
				selectEasing.sizeTime = 0.0f;
				selectEasing.sizeEasedT = 0.0f;
			}
			if (Input::GetInstance()->TriggerKey(DIK_W) || Input::GetInstance()->TriggerKey(DIK_UP)) {
				currentPause_ = Pause::kSelect;
				easingType_ = Select;

				resumeEasing.startSizeV2 = resumeEasing.size;
				resumeEasing.endSizeV2 = {300.0f, 300.0f};
				resumeEasing.sizeTime = 0.0f;
				resumeEasing.sizeEasedT = 0.0f;

				selectEasing.startSizeV2 = selectEasing.size;
				selectEasing.endSizeV2 = {400.0f, 400.0f};
				selectEasing.sizeTime = 0.0f;
				selectEasing.sizeEasedT = 0.0f;
			}
			if (Input::GetInstance()->TriggerKey(DIK_S) || Input::GetInstance()->TriggerKey(DIK_DOWN)) {
				currentPause_ = Pause::kRetry;
				easingType_ = Select;

				resumeEasing.startSizeV2 = resumeEasing.size;
				resumeEasing.endSizeV2 = {300.0f, 300.0f};
				resumeEasing.sizeTime = 0.0f;
				resumeEasing.sizeEasedT = 0.0f;

				retryEasing.startSizeV2 = retryEasing.size;
				retryEasing.endSizeV2 = {400.0f, 400.0f};
				retryEasing.sizeTime = 0.0f;
				retryEasing.sizeEasedT = 0.0f;
			}
		}

		break;
	case Pause::kRetry:
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			// ゲームプレイシーン(次シーン)を生成
			SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
		}
		if (Input::GetInstance()->TriggerKey(DIK_W) || Input::GetInstance()->TriggerKey(DIK_UP)) {
			currentPause_ = Pause::kResume;

			retryEasing.startSizeV2 = retryEasing.size;
			retryEasing.endSizeV2 = {300.0f, 300.0f};
			retryEasing.sizeTime = 0.0f;
			retryEasing.sizeEasedT = 0.0f;

			resumeEasing.startSizeV2 = resumeEasing.size;
			resumeEasing.endSizeV2 = {400.0f, 400.0f};
			resumeEasing.sizeTime = 0.0f;
			resumeEasing.sizeEasedT = 0.0f;
		}
		if (Input::GetInstance()->TriggerKey(DIK_S) || Input::GetInstance()->TriggerKey(DIK_DOWN)) {
			currentPause_ = Pause::kSelect;

			retryEasing.startSizeV2 = retryEasing.size;
			retryEasing.endSizeV2 = {300.0f, 300.0f};
			retryEasing.sizeTime = 0.0f;
			retryEasing.sizeEasedT = 0.0f;

			selectEasing.startSizeV2 = selectEasing.size;
			selectEasing.endSizeV2 = {400.0f, 400.0f};
			selectEasing.sizeTime = 0.0f;
			selectEasing.sizeEasedT = 0.0f;
		}
		break;
	case Pause::kSelect:
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			// ゲームプレイシーン(次シーン)を生成
			SceneManager::GetInstance()->ChangeScene("GAMESELECT");
		}
		if (Input::GetInstance()->TriggerKey(DIK_W) || Input::GetInstance()->TriggerKey(DIK_UP)) {
			currentPause_ = Pause::kRetry;

			selectEasing.startSizeV2 = selectEasing.size;
			selectEasing.endSizeV2 = {300.0f, 300.0f};
			selectEasing.sizeTime = 0.0f;
			selectEasing.sizeEasedT = 0.0f;

			retryEasing.startSizeV2 = retryEasing.size;
			retryEasing.endSizeV2 = {400.0f, 400.0f};
			retryEasing.sizeTime = 0.0f;
			retryEasing.sizeEasedT = 0.0f;
		}
		if (Input::GetInstance()->TriggerKey(DIK_S) || Input::GetInstance()->TriggerKey(DIK_DOWN)) {
			currentPause_ = Pause::kResume;

			selectEasing.startSizeV2 = selectEasing.size;
			selectEasing.endSizeV2 = {300.0f, 300.0f};
			selectEasing.sizeTime = 0.0f;
			selectEasing.sizeEasedT = 0.0f;

			resumeEasing.startSizeV2 = resumeEasing.size;
			resumeEasing.endSizeV2 = {400.0f, 400.0f};
			resumeEasing.sizeTime = 0.0f;
			resumeEasing.sizeEasedT = 0.0f;
		}
		break;
	}

	// イージング更新
	easing->SizeV2(resumeEasing, 0.05f, 1);
	if (resumeEasing.sizeTime >= 0.5f)
		easing->SizeV2(retryEasing, 0.05f, 1);
	if (retryEasing.sizeTime >= 0.5f)
		easing->SizeV2(selectEasing, 0.05f, 1);

	// トランスフォーム更新
	resume_->SetSize(resumeEasing.size);
	retry_->SetSize(retryEasing.size);
	select_->SetSize(selectEasing.size);

	// スプライト更新
	resume_->Update();
	retry_->Update();
	select_->Update();
	pauseBg_->Update();
}

void GamePlayScene::StageClear() {
	if (isFinished_)
		return;
	isFinished_ = true;

	// 現在のゲーム情報を取得
	this->score_+=enemy_->GiveScore();
	std::string currentStage = std::to_string(currentStage_);
	
	std::string currentModel = "normal.obj";
	if (style_ == Style::speed) currentModel = "speed.obj";
	if (style_ == Style::power) currentModel = "power.obj";
	if (style_ == Style::sniper) currentModel = "sniper.obj";

	// 保存実行
	scoreManager_.SaveScene(score_, currentStage, currentModel, playTimer_);

	SceneManager::GetInstance()->ChangeScene("RESULT");
}

void GamePlayScene::LithingEffect() {
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

#pragma endregion

#pragma region レイマーチング

	// レイマーチング
	RayMarching::GetInstance()->Update(camera.get());
	// rayMarching->SetTime(rayMarchingTime);
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
}

void GamePlayScene::UpdateImGui() {
#ifdef USE_IMGUI

	// ImGui
	// フレームレートの取得と表示
	float fps = ImGui::GetIO().Framerate;
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / fps, fps);
	ImGui::Text("time: %.2f", playTimer_);
	// カメラ
	ImGui::DragFloat3("cameraTranslate", &cameraTransform.translate.x, 0.01f, -100.0f, 100.0f);
	ImGui::DragFloat3("cameraRotate", &cameraTransform.rotate.x, 0.01f, -180.0f, 180.0f);
	camera->SetTranslate({cameraTransform.translate});
	camera->SetRotate({cameraTransform.rotate});

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
			ImGui::DragInt("lensFlareGhostCount", &lensFlareGhostCount, 1, 0, 10);
			ImGui::DragFloat("lensFlareHaloWidth", &lensFlareHaloWidth, 0.01f, 0.0f, 10.0f);
			ImGui::Checkbox("isACES", &isACES);
			ImGui::DragFloat("caIntensity", &caIntensity, 0.001f, 0.0f, 10.0f);
		}

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

	cameraController_->EditorUpdate();
	enemy_->DrawImGui();

#pragma endregion

#pragma region レイマーチング

	// レイマーチング
	// ImGui::DragFloat("rayMarchingTime", &rayMarchingTime, 0.1f,0.0f,10.0f);
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

void GamePlayScene::SceneChangedEffect()
{
    // セレクトから遷移した時のエフェクトを元に戻す
    if (isSceneChanged_) {
        if (speedDistortionStrength > 0.0f)
            speedDistortionStrength -= 0.1f;
        else
            isSpeedDistortion = false;

        if (blurWidth > 0.0f)
            blurWidth -= 0.001f;
        else
            isRadialBlur = false;

        if (!isSpeedDistortion && !isRadialBlur)
            isSceneChanged_ = false;
    }
}
