#include "GamePlayScene.h"
#include "ObjectCommon.h"
#include "SceneManager.h"
#include "ScoreManager.h"
#include "SpriteCommon.h"

void GamePlayScene::Initialize()
{
    // カメラ初期化
    camera = std::make_unique<Camera>();
    camera->SetRotate({ cameraTransform.rotate });
    camera->SetTranslate({ cameraTransform.translate });

    // カメラマネージャ登録
    CameraManager::GetInstance()->AddCamera("main", camera.get());
    CameraManager::GetInstance()->SetActiveCamera("main");

    cameraController_ = std::make_unique<CameraController>();
    cameraController_->Initialize(camera.get());

    player_ = std::make_unique<Player>();
    player_->Initialize(camera.get(), style_);

    enemy_ = std::make_unique<EnemyManager>();
    enemy_->Initialize(player_.get(), camera.get(), cameraController_.get());
    cameraController_->SetCurrentStage(currentStage_);
    cameraController_->StartReplay();

    // スプライト
    pause_ = std::make_unique<Sprite>();
    pause_->Initialize("Resource/pause.png"); // ポーズ
    pause_->SetPosition({ 100.0f, 50.0f }); // 画面中央

    resume_ = std::make_unique<Sprite>();
    resume_->Initialize("Resource/resume.png"); // 続ける
    resume_->SetPosition({ 960.0f, 216.0f });
    resumeEasing.size = { 0.0f, 0.0f };
    resumeEasing.startSizeV2 = { 0.0f, 0.0f };
    resumeEasing.endSizeV2 = { 400.0f, 400.0f };
    resumeEasing.sizeTime = 0.0f;
    resumeEasing.sizeEasedT = 0.0f;

    retry_ = std::make_unique<Sprite>();
    retry_->Initialize("Resource/retry.png"); // リトライ
    retry_->SetPosition({ 860.0f, 432.0f });
    retryEasing.size = { 0.0f, 0.0f };
    retryEasing.startSizeV2 = { 0.0f, 0.0f };
    retryEasing.endSizeV2 = { 300.0f, 300.0f };
    retryEasing.sizeTime = 0.0f;
    retryEasing.sizeEasedT = 0.0f;

    select_ = std::make_unique<Sprite>();
    select_->Initialize("Resource/select.png"); // セレクトへ
    select_->SetPosition({ 1060.0f, 648.0f });
    selectEasing.size = { 0.0f, 0.0f };
    selectEasing.startSizeV2 = { 0.0f, 0.0f };
    selectEasing.endSizeV2 = { 300.0f, 300.0f };
    selectEasing.sizeTime = 0.0f;
    selectEasing.sizeEasedT = 0.0f;

    // playerHPバー
    playerHpUI_ = std::make_unique<Sprite>();
    playerHpUI_->Initialize("Resource/UI/playerHp.png");
    playerHpUI_->SetPosition({ 100.0f, 50.0f });

    pauseBg_ = std::make_unique<Sprite>();
    pauseBg_->Initialize("Resource/pauseBg.png"); // ポーズ背景
    pauseBg_->SetPosition({ 960.0f, 540.0f });
    pauseBg_->SetSize({ 1920.0f, 1080.0f });

    // イージング
    easing = std::make_unique<Easing>();
    easing->Initialize();
}

void GamePlayScene::Update()
{

    if (!isPause_) {
        // セレクトからシーン切り替えした時のエフェクト
        SceneChangedEffect();

        // カメラ更新
        cameraController_->Update();

        // プレイヤー更新
        player_->Update(enemy_->GetEnemies(), cameraController_->GetSpeed());

        // 敵更新
        enemy_->SetcurrentTimer_(cameraController_->GetElapsedTime());
        enemy_->Update();

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

    // 時間の計測
    float deltaTime = 1.0f / 60.0f;
    playTimer_ += deltaTime;

    // クリア条件の分岐
    if (isBossBattle_) {
        // ボス倒したらクリア
    } else { // 制限時間来たらリザルトへ
        if (playTimer_ >= kMaxTime_) {
            playTimer_ = kMaxTime_;
            StageClear();
        }
    }

    // スプライト更新
    pause_->Update();

    LithingEffect();
    UpdateImGui();

    // イージング更新
    easing->Update();
    easing->Draw();
}

void GamePlayScene::Draw2D()
{
    // 2Dオブジェクトの描画準備
    SpriteCommon::GetInstance()->SetCommonPipelineState();

    player_->Draw2D();

    pause_->Draw(); // ポーズ

    playerHpUI_->Draw();

    if (isPause_) {
        pauseBg_->Draw(); // ポーズ背景
        resume_->Draw(); // ポーズ//続ける
        retry_->Draw(); // リトライ
        select_->Draw(); // セレクトへ
    }
}

void GamePlayScene::Draw3D()
{
    // 3Dオブジェクトの描画準備
    ObjectCommon::GetInstance()->SetCommonPipelineState();
    // 3Dオブジェクト描画
    player_->Draw3D();

    enemy_->Draw3D();

    cameraController_->EditorDraw();

    // パーティクル描画
    // ParticleManager::GetInstance()->Draw();

    // アウトライン描画準備
    ObjectCommon::GetInstance()->SetOutlinePipelineState();

    // player_->Draw3D();

    // アウトライン描画
    // object->Draw();
}

void GamePlayScene::Finalize() { CameraManager::GetInstance()->RemoveCamera("main"); }

void GamePlayScene::SetPlayerStyle(int style) { style_ = static_cast<Style>(style); }

void GamePlayScene::SetCurrentStage(int currentStage) { currentStage_ = currentStage; }

void GamePlayScene::ChekeAllCollision()
{
    const std::list<std::shared_ptr<Enemy>>& enemies = enemy_->GetEnemies();
    const std::list<std::shared_ptr<grapesBoss>>& Boss = enemy_->GetGBoss();
    const std::list<std::shared_ptr<banana>>& BBoss = enemy_->GetBBoss();
    CheckCollisionPlayerEnemy(player_.get(), enemies);
    CheckCollisionPlayerEnemyBullet(player_.get(), enemies);
    CheckCollisionPlayerBulletEnemy(player_.get(), enemies);
    CheckCollisionPlayerBulletBossEnemy(player_.get(), Boss);
    CheckCollisionPlayerBossEnemy(player_.get(), Boss);
    CheckCollisionPlayerBossEnemyBullet(player_.get(), Boss);
    CheckCollisionPlayerBulletBananaBoss(player_.get(), BBoss);
    CheckCollisionPlayerBananaBoss(player_.get(), BBoss);
    CheckCollisionPlayerBananaBossBullet(player_.get(), BBoss);
    if (player_->GetIsSpecialAttack() && specialAttackTimer <= 0) {
        CheckCollisionSpecialAtackEnemy(enemies);
        specialAttackTimer = 60; // 特殊攻撃のエフェクト時間（例: 60フレーム）
    }
    if (specialAttackTimer > 0) {
        specialAttackTimer--;
        if (specialAttackTimer <= 0) {
            player_->SetIsSpecialAttack(false); // 特殊攻撃の当たり判定は1フレームだけ

            specialAttackTimer = 0; // タイマーリセット
        }
    }
}

// ポーズ選択
void GamePlayScene::PauseSelect()
{
    if (resumeEasing.sizeTime >= 1.0f && retryEasing.sizeTime >= 1.0f && selectEasing.sizeTime >= 1.0f)
        if (Input::GetInstance()->TriggerKey(DIK_ESCAPE) || Input::GetInstance()->TriggerKey(DIK_SPACE)) {
            isPauseEasing_ = true;
            easingType_ = Start;

            resumeEasing.startSizeV2 = resumeEasing.size;
            resumeEasing.endSizeV2 = { 0.0f, 0.0f };
            resumeEasing.sizeTime = 0.0f;
            resumeEasing.sizeEasedT = 0.0f;

            retryEasing.startSizeV2 = retryEasing.size;
            retryEasing.endSizeV2 = { 0.0f, 0.0f };
            retryEasing.sizeTime = 0.0f;
            retryEasing.sizeEasedT = 0.0f;

            selectEasing.startSizeV2 = selectEasing.size;
            selectEasing.endSizeV2 = { 0.0f, 0.0f };
            selectEasing.sizeTime = 0.0f;
            selectEasing.sizeEasedT = 0.0f;
        }

    if (isPauseEasing_) {
        if (selectEasing.sizeTime >= 1.0f) {
            isPause_ = false;
            isPauseEasing_ = false;

            resumeEasing.startSizeV2 = resumeEasing.size;
            resumeEasing.endSizeV2 = { 400.0f, 400.0f };
            resumeEasing.sizeTime = 0.0f;
            resumeEasing.sizeEasedT = 0.0f;

            retryEasing.startSizeV2 = retryEasing.size;
            retryEasing.endSizeV2 = { 300.0f, 300.0f };
            retryEasing.sizeTime = 0.0f;
            retryEasing.sizeEasedT = 0.0f;

            selectEasing.startSizeV2 = selectEasing.size;
            selectEasing.endSizeV2 = { 300.0f, 300.0f };
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
                resumeEasing.endSizeV2 = { 0.0f, 0.0f };
                resumeEasing.sizeTime = 0.0f;
                resumeEasing.sizeEasedT = 0.0f;

                retryEasing.startSizeV2 = retryEasing.size;
                retryEasing.endSizeV2 = { 0.0f, 0.0f };
                retryEasing.sizeTime = 0.0f;
                retryEasing.sizeEasedT = 0.0f;

                selectEasing.startSizeV2 = selectEasing.size;
                selectEasing.endSizeV2 = { 0.0f, 0.0f };
                selectEasing.sizeTime = 0.0f;
                selectEasing.sizeEasedT = 0.0f;
            }
            if (Input::GetInstance()->TriggerKey(DIK_W) || Input::GetInstance()->TriggerKey(DIK_UP)) {
                currentPause_ = Pause::kSelect;
                easingType_ = Select;

                resumeEasing.startSizeV2 = resumeEasing.size;
                resumeEasing.endSizeV2 = { 300.0f, 300.0f };
                resumeEasing.sizeTime = 0.0f;
                resumeEasing.sizeEasedT = 0.0f;

                selectEasing.startSizeV2 = selectEasing.size;
                selectEasing.endSizeV2 = { 400.0f, 400.0f };
                selectEasing.sizeTime = 0.0f;
                selectEasing.sizeEasedT = 0.0f;
            }
            if (Input::GetInstance()->TriggerKey(DIK_S) || Input::GetInstance()->TriggerKey(DIK_DOWN)) {
                currentPause_ = Pause::kRetry;
                easingType_ = Select;

                resumeEasing.startSizeV2 = resumeEasing.size;
                resumeEasing.endSizeV2 = { 300.0f, 300.0f };
                resumeEasing.sizeTime = 0.0f;
                resumeEasing.sizeEasedT = 0.0f;

                retryEasing.startSizeV2 = retryEasing.size;
                retryEasing.endSizeV2 = { 400.0f, 400.0f };
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
            retryEasing.endSizeV2 = { 300.0f, 300.0f };
            retryEasing.sizeTime = 0.0f;
            retryEasing.sizeEasedT = 0.0f;

            resumeEasing.startSizeV2 = resumeEasing.size;
            resumeEasing.endSizeV2 = { 400.0f, 400.0f };
            resumeEasing.sizeTime = 0.0f;
            resumeEasing.sizeEasedT = 0.0f;
        }
        if (Input::GetInstance()->TriggerKey(DIK_S) || Input::GetInstance()->TriggerKey(DIK_DOWN)) {
            currentPause_ = Pause::kSelect;

            retryEasing.startSizeV2 = retryEasing.size;
            retryEasing.endSizeV2 = { 300.0f, 300.0f };
            retryEasing.sizeTime = 0.0f;
            retryEasing.sizeEasedT = 0.0f;

            selectEasing.startSizeV2 = selectEasing.size;
            selectEasing.endSizeV2 = { 400.0f, 400.0f };
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
            selectEasing.endSizeV2 = { 300.0f, 300.0f };
            selectEasing.sizeTime = 0.0f;
            selectEasing.sizeEasedT = 0.0f;

            retryEasing.startSizeV2 = retryEasing.size;
            retryEasing.endSizeV2 = { 400.0f, 400.0f };
            retryEasing.sizeTime = 0.0f;
            retryEasing.sizeEasedT = 0.0f;
        }
        if (Input::GetInstance()->TriggerKey(DIK_S) || Input::GetInstance()->TriggerKey(DIK_DOWN)) {
            currentPause_ = Pause::kResume;

            selectEasing.startSizeV2 = selectEasing.size;
            selectEasing.endSizeV2 = { 300.0f, 300.0f };
            selectEasing.sizeTime = 0.0f;
            selectEasing.sizeEasedT = 0.0f;

            resumeEasing.startSizeV2 = resumeEasing.size;
            resumeEasing.endSizeV2 = { 400.0f, 400.0f };
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

void GamePlayScene::StageClear()
{
    if (isFinished_)
        return;
    isFinished_ = true;

    // 現在のゲーム情報を取得
    int fianalScore = this->score_;
    std::string currentStage = "Stage " + std::to_string(currentStage_);
    std::string currentModel = "cloud";

    // 保存実行
    scoreManager_.SaveScene(fianalScore, currentStage, currentModel, playTimer_);

    SceneManager::GetInstance()->ChangeScene("RESULT");
}

void GamePlayScene::LithingEffect()
{
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
    if (isSceneChanged_) {
        // 色収差
        PostEffect::GetInstance()->SetFullScreenCA(isFullScreenCA);
        PostEffect::GetInstance()->SetFullScreenCAIntensity(fullScreenCAIntensity);
        // スピードディストーション
        PostEffect::GetInstance()->SetSpeedDistortion(isSpeedDistortion);
        PostEffect::GetInstance()->SetSpeedDistortionStrength(speedDistortionStrength);
    }
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

void GamePlayScene::UpdateImGui()
{
#ifdef USE_IMGUI

    // ImGui
    // フレームレートの取得と表示
    float fps = ImGui::GetIO().Framerate;
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / fps, fps);
    ImGui::Text("time: %.2f", playTimer_);
    // カメラ
    ImGui::DragFloat3("cameraTranslate", &cameraTransform.translate.x, 0.01f, -100.0f, 100.0f);
    ImGui::DragFloat3("cameraRotate", &cameraTransform.rotate.x, 0.01f, -180.0f, 180.0f);
    camera->SetTranslate({ cameraTransform.translate });
    camera->SetRotate({ cameraTransform.rotate });

    if (Input::GetInstance()->TriggerKey(DIK_BACKSPACE)) {
        // ゲームプレイシーン(次シーン)を生成
        SceneManager::GetInstance()->ChangeScene("RESULT");
    }

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
        if (fullScreenCAIntensity > 0.0f)
            fullScreenCAIntensity -= 0.05f;
        else
            isFullScreenCA = false;

        if (speedDistortionStrength > 0.0f)
            speedDistortionStrength -= 0.1f;
        else
            isSpeedDistortion = false;

        if (blurWidth > 0.0f)
            blurWidth -= 0.001f;
        else
            isRadialBlur = false;

        if (!isFullScreenCA && !isSpeedDistortion && !isRadialBlur)
            isSceneChanged_ = false;
    }
}
