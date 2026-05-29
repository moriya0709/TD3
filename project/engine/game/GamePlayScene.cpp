#include "GamePlayScene.h"
#include "BananaCameraController.h"
#include "GrapeCameraController.h"
#include "Model.h"
#include "ObjectCommon.h"
#include "SceneManager.h"
#include "ScoreManager.h"
#include "SpriteCommon.h"
#include "StageCameraController.h"

void GamePlayScene::Initialize()
{
    // カメラ初期化
    camera = std::make_unique<Camera>();
    camera->SetRotate({ cameraTransform.rotate });
    camera->SetTranslate({ cameraTransform.translate });

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
    skeleton_ = model->CreateSkeleton(model->GetModelData().rootNode); // 動く仕組み

    // アニメーションデータの読み込み(モデル自体はGame.cppに入れること)
    simpleAnimation_ = Model::LoadAnimationFile("./Resource", "simpleSkin.gltf"); // スケルトン
    walkAnimation_ = Model::LoadAnimationFile("./Resource", "walk.gltf");
    ///
    ///
    ///


    for (int i = 0; i < kMaxSpecialAttack; i++) {
        // HPバーの右隣からスタートし、アイコンの幅ごとに右にズラす
        // ※ 260.0f はHPバーの幅(240)＋少しの余白です。アイコンの幅(例:40.0f)を掛けて並べます
        float gaugePosX = 260.0f + (i * 40.0f);
        float gaugePosY = 15.0f; // HPバーのY座標(10.0f)に合わせて調整

        // 必殺技回数ゲージ（溜まっている時）
        gaugeUI_[i] = std::make_unique<Sprite>();
        gaugeUI_[i]->Initialize("Resource/UI/HissatuGageEgg.png");
        gaugeUI_[i]->SetPosition({ gaugePosX, gaugePosY });

        // 必殺技回数空（使った後）
        gaugeEmptyUI_[i] = std::make_unique<Sprite>();
        gaugeEmptyUI_[i]->Initialize("Resource/UI/HissatuNoGage.png");
        gaugeEmptyUI_[i]->SetPosition({ gaugePosX, gaugePosY });
    }

    // スプライト
    pauseM_ = std::make_unique<Sprite>();
    pauseM_->Initialize("Resource/pauseM.png"); // ポーズ
    pauseM_->SetPosition({ 1850.0f, 50.0f });

    pauseC_ = std::make_unique<Sprite>();
    pauseC_->Initialize("Resource/pauseC.png"); // ポーズ
    pauseC_->SetPosition({ 1850.0f, 50.0f });

    resumeM_ = std::make_unique<Sprite>();
    resumeM_->Initialize("Resource/resumeM.png"); // 続ける
    resumeM_->SetPosition({ 960.0f, 216.0f });

    resumeC_ = std::make_unique<Sprite>();
    resumeC_->Initialize("Resource/resumeC.png"); // 続ける
    resumeC_->SetPosition({ 960.0f, 216.0f });

    resumeEasing.size = { 0.0f, 0.0f };
    resumeEasing.startSizeV2 = { 0.0f, 0.0f };
    resumeEasing.endSizeV2 = { 400.0f, 400.0f };
    resumeEasing.sizeTime = 0.0f;
    resumeEasing.sizeEasedT = 0.0f;

    retryM_ = std::make_unique<Sprite>();
    retryM_->Initialize("Resource/retryM.png"); // リトライ
    retryM_->SetPosition({ 860.0f, 432.0f });
    retryEasing.size = { 0.0f, 0.0f };
    retryEasing.startSizeV2 = { 0.0f, 0.0f };
    retryEasing.endSizeV2 = { 300.0f, 300.0f };
    retryEasing.sizeTime = 0.0f;
    retryEasing.sizeEasedT = 0.0f;

    retryC_ = std::make_unique<Sprite>();
    retryC_->Initialize("Resource/retryC.png"); // リトライ
    retryC_->SetPosition({ 860.0f, 432.0f });
    retryEasing.size = { 0.0f, 0.0f };
    retryEasing.startSizeV2 = { 0.0f, 0.0f };
    retryEasing.endSizeV2 = { 300.0f, 300.0f };
    retryEasing.sizeTime = 0.0f;
    retryEasing.sizeEasedT = 0.0f;

    selectM_ = std::make_unique<Sprite>();
    selectM_->Initialize("Resource/selectM.png"); // セレクトへ
    selectM_->SetPosition({ 1060.0f, 648.0f });
    selectEasing.size = { 0.0f, 0.0f };
    selectEasing.startSizeV2 = { 0.0f, 0.0f };
    selectEasing.endSizeV2 = { 300.0f, 300.0f };
    selectEasing.sizeTime = 0.0f;
    selectEasing.sizeEasedT = 0.0f;

    selectC_ = std::make_unique<Sprite>();
    selectC_->Initialize("Resource/selectC.png"); // セレクトへ
    selectC_->SetPosition({ 1060.0f, 648.0f });
    selectEasing.size = { 0.0f, 0.0f };
    selectEasing.startSizeV2 = { 0.0f, 0.0f };
    selectEasing.endSizeV2 = { 300.0f, 300.0f };
    selectEasing.sizeTime = 0.0f;
    selectEasing.sizeEasedT = 0.0f;

    // playerHPバーのUI部分(外枠)
    playerHpUI_ = std::make_unique<Sprite>();
    playerHpUI_->Initialize("Resource/UI/playerHp.png");
    playerHpUI_->SetAnchorPoint({ 0.0f, 0.0f });
    playerHpUI_->SetPosition({ 8.0f, 10.0f });
    playerHpUI_->SetSize({ 240.0f, 50.0f });
    // playerHPのHPゲージ部分
    playerHPGauge_ = std::make_unique<Sprite>();
    playerHPGauge_->Initialize("Resource/white.png");
    playerHPGauge_->SetAnchorPoint({ 0.0f, 0.0f }); // サイズ調整
    playerHPGauge_->SetPosition({ 39.0f, 22.0f }); // UIの透過部分に合うように右に
    // playerHPのゲージが減った時の空部分
    playerHPEmpty_ = std::make_unique<Sprite>();
    playerHPEmpty_->Initialize("Resource/white.png");

    playerHPEmpty_->SetAnchorPoint({ 0.0f, 0.0f }); // サイズ調整
    playerHPEmpty_->SetPosition({ 39.0f, 22.0f }); // UIの透過部分に合うように

    BulletRuleM_ = std::make_unique<Sprite>();
    BulletRuleM_->Initialize("Resource/UI/BulletRuleM.png"); // bulletルールマウス
    BulletRuleM_->SetPosition({ 400.0f, 300.0f });

    BulletRuleC_ = std::make_unique<Sprite>();
    BulletRuleC_->Initialize("Resource/UI/BulletRuleC.png"); // bulletルールコントローラー
    BulletRuleC_->SetPosition({ 400.0f, 300.0f });

    spacialRuleM_ = std::make_unique<Sprite>();
    spacialRuleM_->Initialize("Resource/UI/specialRuleM.png"); // specialルールマウス
    spacialRuleM_->SetPosition({ 1550.0f, 300.0f });

    spacialRuleC_ = std::make_unique<Sprite>();
    spacialRuleC_->Initialize("Resource/UI/specialRuleC.png"); // specialルールコントローラー
    spacialRuleC_->SetPosition({ 1550.0f, 300.0f });

    for (int i = 0; i < kMaxSpecialAttack; i++) {
        // 必殺技回数ゲージ
        gaugeUI_[i] = std::make_unique<Sprite>();
        gaugeUI_[i]->Initialize("Resource/UI/HissatuGageEgg.png");
        gaugeUI_[i]->SetPosition({ 280.0f + i * 60.0f, 40.0f });
        gaugeUI_[i]->SetSize({ 50.0f, 50.0f });
        // 必殺技回数空
        gaugeEmptyUI_[i] = std::make_unique<Sprite>();
        gaugeEmptyUI_[i]->Initialize("Resource/UI/HissatuNoGage.png");
        gaugeEmptyUI_[i]->SetPosition({ 280.0f + i * 60.0f, 40.0f });
        gaugeEmptyUI_[i]->SetSize({ 50.0f, 50.0f });
    }

    pauseBg_ = std::make_unique<Sprite>();
    pauseBg_->Initialize("Resource/pauseBg.png"); // ポーズ背景
    pauseBg_->SetPosition({ 960.0f, 540.0f });
    pauseBg_->SetSize({ 1920.0f, 1080.0f });

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
    gameOverUi_[0]->SetSize({ 300.0f, 300.0f });
    gameOverUi_[1]->SetSize({ 300.0f, 300.0f });
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

    // *ボス登場演出* //

    // 警告ライン
    warningLinePos_[0] = { 960.0f, 540.0f };
    warningLinePos_[1] = { 2880.0f, 540.0f };
    for (int i = 0; i < kWarningLine_; i++) {
        warningLine_[i] = std::make_unique<Sprite>();
        warningLine_[i]->Initialize("Resource/BossAppears/warningLine.png");
        warningLine_[i]->SetPosition(warningLinePos_[i]);
    }

    // 警告文字
    warning_ = std::make_unique<Sprite>();
    warning_->Initialize("Resource/BossAppears/warning.png");
    warning_->SetPosition({ 960.0f, 540.0f });
    warning_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });

    // 警告文字のイージング
    warningEasing_.startColor = { 1.0f, 1.0f, 1.0f, 0.0f };
    warningEasing_.endColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    warningEasing_.colorTime = 0.0f;
    warningEasing_.colorEasedT = 0.0f;

    // ブドウモデル
    for (int i = 0; i < kBossAppearsGrapes_; i++) {
        bossAppearsGrapes_[i] = std::make_unique<Object>();
        bossAppearsGrapes_[i]->Initialize(camera.get());
        bossAppearsGrapes_[i]->SetScale({ 14.0f, 14.0f, 14.0f });
        bossAppearsGrapes_[i]->SetRotate({ 0.0f, 3.14f, 0.0f });

        if (i != 0)
            bossAppearsGrapes_[i]->SetModel("bossGrapesOnly.obj");

        grapesEasing_[i].startRotation = { 0.0f, 3.14f, 0.0f };
        grapesEasing_[i].endRotation = { 0.0f, 3.14f, 0.0f };
        grapesEasing_[i].moveTime = 0.0f;
        grapesEasing_[i].rotationTime = 0.0f;
        grapesEasing_[i].moveEasedT = 0.0f;
        grapesEasing_[i].rotationEasedT = 0.0f;
    }
    bossAppearsGrapes_[2]->SetRotate({ 0.0f, 0.0f, 0.0f });
    bossAppearsGrapes_[0]->SetModel("bossGrapesBranch.obj");

    // ブドウのイージングの開始位置と終了位置
    grapesEasing_[0].startPos = { 0.0f, 30.0f, 30.0f };
    grapesEasing_[1].startPos = { -30.0f, 10.0f, 30.0f };
    grapesEasing_[2].startPos = { 0.0f, 30.0f, 30.0f };
    grapesEasing_[3].startPos = { 30.0f, 10.0f, 30.0f };
    grapesEasing_[4].startPos = { -30.0f, 0.0f, 30.0f };
    grapesEasing_[5].startPos = { 30.0f, 0.0f, 30.0f };
    grapesEasing_[6].startPos = { 0.0f, -30.0f, 30.0f };

    grapesEasing_[2].startRotation = { 0.0f, 0.0f, 0.0f };
    grapesEasing_[5].startRotation = { 0.0f, 3.14f, 0.0f };

    grapesEasing_[0].endPos = { 0.0f, 8.0f, 30.0f };
    grapesEasing_[1].endPos = { -5.0f, 5.0f, 30.0f };
    grapesEasing_[2].endPos = { 0.0f, 5.0f, 30.0f };
    grapesEasing_[3].endPos = { 5.0f, 5.0f, 30.0f };
    grapesEasing_[4].endPos = { -2.5f, 0.0f, 30.0f };
    grapesEasing_[5].endPos = { 2.5f, 0.0f, 30.0f };
    grapesEasing_[6].endPos = { 0.0f, -5.0f, 30.0f };

    grapesEasing_[2].endRotation = { 0.0f, 3.14f, 0.0f };
    grapesEasing_[5].endRotation = { 0.0f, 0.0f, 0.0f };

    // バナナモデル
    for (int i = 0; i < kBossAppearsBanana_; i++) {
        bossAppearsBanana_[i] = std::make_unique<Object>();
        bossAppearsBanana_[i]->Initialize(camera.get());
        bossAppearsBanana_[i]->SetScale({ 14.0f, 14.0f, 14.0f });
        bananaEasing_[i].endPos = { 0.0f, 0.0f, 30.0f };
        bananaEasing_[i].moveTime = 0.0f;
        bananaEasing_[i].moveEasedT = 0.0f;
    }
    bossAppearsBanana_[0]->SetModel("bossBananBody.obj");
    bossAppearsBanana_[1]->SetModel("bossBananPeelBuck.obj");
    bossAppearsBanana_[2]->SetModel("bossBananPeelLeft.obj");
    bossAppearsBanana_[3]->SetModel("bossBananPeelRight.obj");

    bananaEasing_[0].startPos = { 0.0f, 30.0f, 30.0f };
    bananaEasing_[1].startPos = { -30.0f, 0.0f, 30.0f };
    bananaEasing_[2].startPos = { 0.0f, 0.0f, -10.0f };
    bananaEasing_[3].startPos = { 30.0f, 0.0f, 30.0f };

    // ボスの名前表示
    for (int i = 0; i < 2; i++) {
        bossAppearsName_[i] = std::make_unique<Sprite>();
        if (i == 0)
            bossAppearsName_[0]->Initialize("Resource/BossAppears/grapeName.png");
        else
            bossAppearsName_[1]->Initialize("Resource/BossAppears/bananaName.png");
        bossAppearsName_[i]->SetPosition({ 960.0f, 740.0f });
        bossAppearsName_[i]->SetSize({ 300.0f, 300.0f });
    }

    nameEasing_.startSizeV2 = { 0.0f, 0.0f };
    nameEasing_.endSizeV2 = { 800.0f, 800.0f };
    nameEasing_.sizeTime = 0.0f;
    nameEasing_.sizeEasedT = 0.0f;

    // イージング
    easing = std::make_unique<Easing>();
    easing->Initialize();

    // 音声再生
    SoundManager::GetInstance()->Play("stage.mp3",true,bgmVolume_);

}

void GamePlayScene::Update()
{

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

        if (isBossAppears_) {
            // ボス登場演出
            BossAppearsUpdate();
        } else {
            // プレイヤー更新
            player_->Update(enemy_->GetEnemies(), cameraController_->GetSpeed());

            // 敵更新
            enemy_->SetcurrentTimer_(cameraController_->GetElapsedTime());
            enemy_->Update();
        }

        // 敵から回収したスコアを自分のスコアに加算する
        this->score_ += enemy_->GiveScore();

        // 当たり判定
        ChekeAllCollision();

        // フェード
        if (isFinished_) {
            // フェードアウト
            intensity = (std::max)(0.0f, intensity - 1.0f / 30.0f);
            PostEffect::GetInstance()->SetIntensity(intensity);

			// BGMのフェードアウト
            if (bgmVolume_ > 0.0f) {
                bgmVolume_ -= 1.0f / 30.0f;
                // BGMのフェードアウト処理
                bgmVolume_ = (std::max)(0.0f, bgmVolume_ - 1.0f / 30.0f); // 音量も同じ速度で減少
                SoundManager::GetInstance()->SetVolume("stage.mp3", bgmVolume_);
                SoundManager::GetInstance()->SetVolume("boss.mp3", bgmVolume_);
            }
            else {
                SoundManager::GetInstance()->Stop("stage.mp3");
                SoundManager::GetInstance()->Stop("boss.mp3");
            }

            if (intensity <= 0.0f)
                SceneManager::GetInstance()->ChangeScene("RESULT");
        } else {
			// フェードイン
            if (!isBossAppears_ && !isWarning_) {
                if (intensity < 1.0f)
                    intensity += 1.0f / 30.0f;
            }
        }

        // ポーズ画面へ
        if (Input::GetInstance()->TriggerKey(DIK_ESCAPE) || Input::GetInstance()->IsPadButtonPressed(0, 7)) { // クロスボタン
            isPause_ = true;
            currentPause_ = Pause::kResume;
        }

    } else { // ポーズ画面
        PauseSelect();

        // エフェクトをリセット
        isInversion = false; // 反転
        isGrayscale = false; // グレースケール
        isTwoColor = false;
        isRadialBlur = false; // 放射線ブラー
        isSpeedDistortion = false; // スピードディストーション
        isConcentrationLines = false; // 集中線
        isFullScreenCA = false; // 色収差
        isVignette = false; // ビネット

        PostEffect::GetInstance()->SetInversion(isInversion);
        PostEffect::GetInstance()->SetGrayscale(isGrayscale);
        PostEffect::GetInstance()->SetTwoColor(isTwoColor);
        PostEffect::GetInstance()->SetRadialBlur(isRadialBlur);
        PostEffect::GetInstance()->SetSpeedDistortion(isSpeedDistortion);
        PostEffect::GetInstance()->SetConcentrationLines(isConcentrationLines);
        PostEffect::GetInstance()->SetFullScreenCA(isFullScreenCA);
        PostEffect::GetInstance()->SetVignette(isVignette);
    }
    int currentSpecialCount = player_->GetSpecialAttackCount();

    for (int i = 0; i < kMaxSpecialAttack; i++) {
        if (i < currentSpecialCount) {
            // 残弾数より小さいインデックスなら、満タンのゲージを描画
            gaugeUI_[i]->Update();
        } else {
            // それ以外（使ってしまった分）は空のゲージを描画
            gaugeEmptyUI_[i]->Update();
        }
    }

    if (isPause_ || isFinished_)
        return;

    // hpが0以下にならないようにclamp
    float hpRate = std::clamp((float)player_->GetHP() / (float)maxHP_, 0.0f, 1.0f);
    float maxBarWidth = 194.0f; // 枠に収まる最大幅
    // ゲージサイズを設定{横幅, 縦幅}
    playerHPGauge_->SetSize({ maxBarWidth * hpRate, 30.0f });
    playerHPEmpty_->SetSize({ maxBarWidth, 30.0f });

    // --- 追加：HPバーの透明度調整 ---
    float hpAlpha = 1.0f; // 基本は不透明 (1.0)

    // プレイヤーの3D座標を2Dの画面座標に変換
    Vector2 playerScreenPos = camera->WorldToScreen(player_->GetPosition());

    // プレイヤーがHPバーの近く(画面左上)にいるか判定
    // (HPバーのサイズや位置に合わせて判定範囲は調整してください)
    if (playerScreenPos.x < 300.0f && playerScreenPos.y < 150.0f) {
        hpAlpha = 0.3f; // 近づいたら透明度を下げる (0.0=透明, 1.0=不透明)
    }

    // 外枠、ゲージ、空部分の色(RGB)と透明度(Alpha)を設定
    playerHpUI_->SetColor(Vector4(1.0f, 1.0f, 1.0f, hpAlpha)); // 外枠
    playerHPGauge_->SetColor(Vector4(0.0f, 1.0f, 1.0f, hpAlpha)); // 水色(ゲージ部分)
    playerHPEmpty_->SetColor(Vector4(0.2f, 0.2f, 0.2f, hpAlpha)); // 暗いグレー(空部分)
    // ---------------------------------

    // クリア条件の分岐
    if (isBossBattle_) {
        if (cameraController_->GetElapsedTime() >= kMaxTime_) {
            if (bossPopFlag == 2) {
                isWarning_ = true;
                if (isWarning_) {
                    WarningEffect();
                    bossAppearsState_ = Grapes;
                }

                enemy_->SetEnemyclear();

                if (!isWarning_) {
                    cameraController_ = std::make_unique<GrapeCameraController>();
                    cameraController_->Initialize(camera.get());
                    enemy_->Initialize(player_.get(), camera.get(), cameraController_.get());
                    bossPopFlag = 4;

                    // bgm
                    bgmVolume_ = 1.0f;
                    SoundManager::GetInstance()->Play("boss.mp3", true, bgmVolume_);
                }

            } else if (bossPopFlag == 3) {
                isWarning_ = true;
                if (isWarning_) {
                    WarningEffect();
                    bossAppearsState_ = Banana;
                }

                enemy_->SetEnemyclear();

                if (!isWarning_) {
                    cameraController_ = std::make_unique<BananaCameraController>();
                    cameraController_->Initialize(camera.get());
                    cameraController_->SetTargetPosition({ 0, 0, 60 });
                    enemy_->Initialize(player_.get(), camera.get(), cameraController_.get());
                    bossPopFlag = 6;

					// bgm
                    bgmVolume_ = 1.0f;
                    SoundManager::GetInstance()->Play("boss.mp3", true, bgmVolume_);
                }

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
                    // 【左への入力】Aキー、左矢印、十字キー左、左スティック左
                    if (Input::GetInstance()->TriggerKey(DIK_A) || Input::GetInstance()->TriggerKey(DIK_LEFT) || Input::GetInstance()->GetPadLeftAxisX(0) < -0.5f) {

                        gameOverUi_[0]->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
                        gameOverUi_[1]->SetColor({ 0.1f, 0.1f, 0.1f, 1.0f });

                        currentGameOverUI_ = Pause::kRetry;
                    }
                    // 【右への入力】Dキー、右矢印、十字キー右、左スティック右
                    if (Input::GetInstance()->TriggerKey(DIK_D) || Input::GetInstance()->TriggerKey(DIK_RIGHT) || Input::GetInstance()->GetPadLeftAxisX(0) > 0.5f) {

                        gameOverUi_[0]->SetColor({ 0.1f, 0.1f, 0.1f, 1.0f });
                        gameOverUi_[1]->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

                        currentGameOverUI_ = Pause::kSelect;
                    }
                }
            }

            // 【決定】スペースキー、またはBボタン（1番）
            if (Input::GetInstance()->TriggerKey(DIK_SPACE) || Input::GetInstance()->IsPadButtonPressed(0, 1)) {
				isPauseSceneChange_ = true;
            }

            if (isPauseSceneChange_) {
                if (currentGameOverUI_ == Pause::kSelect) {
                    // BGMのフェードアウト
					if (bgmVolume_ > 0.0f) {
                        bgmVolume_ -= 1.0f / 30.0f;
                        SoundManager::GetInstance()->SetVolume("stage.mp3", bgmVolume_);
                        SoundManager::GetInstance()->SetVolume("boss.mp3", bgmVolume_);
                    } else {
                        // セレクトシーンを生成
                        SoundManager::GetInstance()->Stop("stage.mp3");
                        SoundManager::GetInstance()->Stop("boss.mp3");

                        SceneManager::GetInstance()->ChangeScene("GAMESELECT");
                    }
                } else if (currentGameOverUI_ == Pause::kRetry) {
                    // BGMのフェードアウト
                    if (bgmVolume_ > 0.0f) {
                        bgmVolume_ -= 1.0f / 30.0f;
                        SoundManager::GetInstance()->SetVolume("stage.mp3", bgmVolume_);
                        SoundManager::GetInstance()->SetVolume("boss.mp3", bgmVolume_);
                    } else {
                        // ゲームプレイシーンを生成
                        SoundManager::GetInstance()->Stop("stage.mp3");
                        SoundManager::GetInstance()->Stop("boss.mp3");

                        SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
                    }
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
    if (Input::GetInstance()->GetCurrentDevice() == InputDevice::Gamepad)
    {
    pauseC_->Update();
    } else
    {
        pauseM_->Update();
    }

    playerHpUI_->Update();
    playerHPEmpty_->Update();
    playerHPGauge_->Update();
    LithingEffect();
    UpdateImGui();

    // これを消すとボス登場演出がバグる
    camera->SetTranslate({ cameraTransform.translate });
    camera->SetRotate({ cameraTransform.rotate });

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

void GamePlayScene::Draw2D()
{
    // 2Dオブジェクトの描画準備
    SpriteCommon::GetInstance()->SetCommonPipelineState();

    if (isBossAppears_) {
        if (bossAppearsState_ == Grapes)
            bossAppearsName_[0]->Draw();
        if (bossAppearsState_ == Banana)
            bossAppearsName_[1]->Draw();
    } else {
        if (!isPause_) // ポーズ中はレティクルを描画しない
            player_->Draw2D();
    }

    playerHpUI_->Draw();
    playerHPEmpty_->Draw();
    playerHPGauge_->Draw();

    // ボス登場演出
    if (isWarning_) {
        for (int i = 0; i < kWarningLine_; i++) {
            warningLine_[i]->Draw();
        }
        warning_->Draw();
    }

    // --- 修正後（GamePlayScene::Draw2D 内） ---

    // ※プレイヤーの残弾数を取得する関数（GetSpecialAttackCountなど）がある前提です
    // もし別の変数名で管理している場合は、それに置き換えてください
    int currentSpecialCount = player_->GetSpecialAttackCount();

    for (int i = 0; i < kMaxSpecialAttack; i++) {
        if (i < currentSpecialCount) {
            // 残弾数より小さいインデックスなら、満タンのゲージを描画
            gaugeUI_[i]->Draw();
        } else {
            // それ以外（使ってしまった分）は空のゲージを描画
            gaugeEmptyUI_[i]->Draw();
        }
    }

    if (player_->GetHP() <= 0) {
        gameOver_->Draw();
        for (int i = 0; i < 2; i++) {
            gameOverUi_[i]->Draw();
        }
    }

    if (Input::GetInstance()->GetCurrentDevice() == InputDevice::Gamepad)
    {
        pauseC_->Draw();
    }
    else {
        pauseM_->Draw();
    }

    if (isPause_) {
        pauseBg_->Draw(); // ポーズ背景

        //操作UI
        if (Input::GetInstance()->GetCurrentDevice() == InputDevice::Gamepad)
        {
            resumeC_->Draw(); // ポーズ//続ける
            retryC_->Draw(); // リトライ
            selectC_->Draw(); // セレクトへ
            BulletRuleC_->Draw();
            spacialRuleC_->Draw();
        } else
        {
            resumeM_->Draw(); // ポーズ//続ける
            retryM_->Draw(); // リトライ
            selectM_->Draw(); // セレクトへ
            //操作説明
            BulletRuleM_->Draw();
            spacialRuleM_->Draw();
        }
    }
}

void GamePlayScene::Draw3D()
{
    // 3Dオブジェクトの描画準備
    ObjectCommon::GetInstance()->SetCommonDrawSetting();
    // 3Dオブジェクト描画
    if (!isBossAppears_) {
        player_->Draw3D();
        enemy_->Draw3D();
    }

    // ボス登場演出
    if (isBossAppears_) {
        if (bossAppearsState_ == Grapes) {
            for (int i = 0; i < kBossAppearsGrapes_; i++)
                bossAppearsGrapes_[i]->Draw();
        } else if (bossAppearsState_ == Banana) {
            for (int i = 0; i < kBossAppearsBanana_; i++)
                bossAppearsBanana_[i]->Draw();
        }
    }

    cameraController_->EditorDraw();

    // アニメーションモデル描画
    ObjectCommon::GetInstance()->SetSkinningCommonDrawSetting();

    // アニメーションモデルの描画
    // for (auto& object : animationObjects) {
    //	if (object->IsSkeletal()) {
    //		object->Draw();
    //	}
    //}

    // パーティクル描画
    // ParticleManager::GetInstance()->Draw();

    // アウトライン描画準備
    ObjectCommon::GetInstance()->SetOutlinePipelineState();

    // player_->Draw3D();

    // アウトライン描画
    // object->Draw();
}

void GamePlayScene::Finalize()
{
    CameraManager::GetInstance()->RemoveCamera("main");
}

void GamePlayScene::SetPlayerStyle(int style) { style_ = static_cast<Style>(style); }

void GamePlayScene::SetCurrentStage(int currentStage) { currentStage_ = currentStage; }

void GamePlayScene::ChekeAllCollision()
{
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
        CheckCollisionSpecialAtacgrapesEnemy(Boss, hitEffect);
        CheckCollisionSpecialAtackbananaEnemy(BBoss, hitEffect);
        specialAttackTimer = 60; // 特殊攻撃のエフェクト時間（例: 60フレーム）

        // エフェクト初期化
        isInversion = true; // 反転エフェクトa
        isGrayscale = true; // グレースケールエフェクト
        isTwoColor = true; // 2色エフェクト
        isConcentrationLines = true; // 集中線エフェクト
        concentrationLineIntensity = 0.5f; // 線の濃さ
        concentrationLineDensity = 1000.0f; // 線の密度（本数）
        concentrationLineLength = 0.0f; // 線の長さ（中心からの開始距離 0.0〜1.0）
        isInversion = true;

        // SE
        SoundManager::GetInstance()->Play("specialAtack_se", false, seVolume_);
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
            isInversion = false; // 反転エフェクト
            isGrayscale = false; // グレースケールエフェクト
            isTwoColor = false; // 2色エフェクト
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

void GamePlayScene::PauseSelect()
{
    if (resumeEasing.sizeTime >= 1.0f && retryEasing.sizeTime >= 1.0f && selectEasing.sizeTime >= 1.0f)
        if (Input::GetInstance()->TriggerKey(DIK_ESCAPE) || Input::GetInstance()->TriggerKey(DIK_SPACE) || Input::GetInstance()->IsPadButtonPressed(0, 7) || Input::GetInstance()->IsPadButtonPressed(0, 1)) {
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
            if (Input::GetInstance()->TriggerKey(DIK_SPACE) || Input::GetInstance()->IsPadButtonPressed(0, 1)) {
                // ゲームプレイシーン(次シーン)を生成
                SoundManager::GetInstance()->Stop("stage.mp3");
                SoundManager::GetInstance()->Stop("boss.mp3");
                SceneManager::GetInstance()->ChangeScene("GAMEPLAY");

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
            if (Input::GetInstance()->TriggerKey(DIK_W) || Input::GetInstance()->TriggerKey(DIK_UP) || Input::GetInstance()->GetPadLeftAxisY(0) < -0.5f) {
                currentPause_ = Pause::kSelect;

                resumeEasing.startSizeV2 = resumeEasing.size;
                resumeEasing.endSizeV2 = { 300.0f, 300.0f };
                resumeEasing.sizeTime = 0.0f;
                resumeEasing.sizeEasedT = 0.0f;

                selectEasing.startSizeV2 = selectEasing.size;
                selectEasing.endSizeV2 = { 400.0f, 400.0f };
                selectEasing.sizeTime = 0.0f;
                selectEasing.sizeEasedT = 0.0f;
            }
            if (Input::GetInstance()->TriggerKey(DIK_S) || Input::GetInstance()->TriggerKey(DIK_DOWN) || Input::GetInstance()->GetPadLeftAxisY(0) > 0.5f) {
                currentPause_ = Pause::kRetry;

                resumeEasing.startSizeV2 = resumeEasing.size;
                resumeEasing.endSizeV2 = { 300.0f, 300.0f };
                resumeEasing.sizeTime = 0.0f;
                resumeEasing.sizeEasedT = 0.0f;

                retryEasing.startSizeV2 = retryEasing.size;
                retryEasing.endSizeV2 = { 400.0f, 400.0f };
                retryEasing.sizeTime = 0.0f;
                retryEasing.sizeEasedT = 0.0f;
            }
            break;
    case Pause::kSelect:
        if (Input::GetInstance()->TriggerKey(DIK_SPACE) || Input::GetInstance()->IsPadButtonPressed(0, 1)) {
            // ゲームプレイシーン(次シーン)を生成
            SoundManager::GetInstance()->Stop("stage.mp3");
            SoundManager::GetInstance()->Stop("boss.mp3");
            SceneManager::GetInstance()->ChangeScene("GAMESELECT");
        }
        if (Input::GetInstance()->TriggerKey(DIK_W) || Input::GetInstance()->TriggerKey(DIK_UP) || Input::GetInstance()->GetPadLeftAxisY(0) < -0.5f) {
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
        if (Input::GetInstance()->TriggerKey(DIK_S) || Input::GetInstance()->TriggerKey(DIK_DOWN) || Input::GetInstance()->GetPadLeftAxisY(0) > 0.5f) {
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
        }

        break;
    case Pause::kRetry:
        if (Input::GetInstance()->TriggerKey(DIK_SPACE) || Input::GetInstance()->IsPadButtonPressed(0, 1)) {
            // ゲームプレイシーン(次シーン)を生成
            SoundManager::GetInstance()->Stop("stage.mp3");
            SoundManager::GetInstance()->Stop("boss.mp3");
            SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
        }
        if (Input::GetInstance()->TriggerKey(DIK_W) || Input::GetInstance()->TriggerKey(DIK_UP) || Input::GetInstance()->GetPadLeftAxisY(0) < -0.5f) {
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
        if (Input::GetInstance()->TriggerKey(DIK_S) || Input::GetInstance()->TriggerKey(DIK_DOWN) || Input::GetInstance()->GetPadLeftAxisY(0) > 0.5f) {
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
    }

    // イージング更新
    easing->SizeV2(resumeEasing, 0.05f, 1);
    if (resumeEasing.sizeTime >= 0.5f)
        easing->SizeV2(retryEasing, 0.05f, 1);
    if (retryEasing.sizeTime >= 0.5f)
        easing->SizeV2(selectEasing, 0.05f, 1);

    // トランスフォーム更新
    if (Input::GetInstance()->GetCurrentDevice() == InputDevice::Gamepad)
    {
        resumeC_->SetSize(resumeEasing.size);
        retryC_->SetSize(retryEasing.size);
        selectC_->SetSize(selectEasing.size);

        resumeC_->Update();
        retryC_->Update();
        selectC_->Update();

        BulletRuleC_->Update();
        spacialRuleC_->Update();
    } else
    {
        resumeM_->SetSize(resumeEasing.size);
        retryM_->SetSize(retryEasing.size);
        selectM_->SetSize(selectEasing.size);

        resumeM_->Update();
        retryM_->Update();
        selectM_->Update();

        BulletRuleM_->Update();
        spacialRuleM_->Update();
    }
    pauseBg_->Update();

}

void GamePlayScene::StageClear()
{

    if (isFinished_)
        return;
    isFinished_ = true;

    // 現在のゲーム情報を取得
    this->score_ += enemy_->GiveScore();
    std::string currentStage = std::to_string(currentStage_);

    std::string currentModel = "normal.obj";
    if (style_ == Style::speed)
        currentModel = "speed.obj";
    if (style_ == Style::power)
        currentModel = "power.obj";
    if (style_ == Style::sniper)
        currentModel = "sniper.obj";

    // 保存実行
    scoreManager_.SaveScene(score_, currentStage, currentModel, playTimer_);
   
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
    // エフェクトの強さ
    PostEffect::GetInstance()->SetIntensity(intensity);

#pragma endregion

#pragma region レイマーチング

    // レイマーチング
    RayMarching::GetInstance()->Update(camera.get());

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

#pragma endregion

#endif
}

void GamePlayScene::WarningEffect()
{
    // 切り換えクールタイム減少
    warningTimer_ = (std::max)(0.0f, warningTimer_ - 1.0f / 60.0f);

    if (warningTimer_ > 0.0f) {
        for (int i = 0; i < kWarningLine_; i++) {
            warningLinePos_[i].x -= warningLineSpeed_; // ループ

            if (warningLinePos_[i].x < -960.0f) {
                warningLinePos_[i].x = 2880.0f; // 画面右端から少し外に配置
            }

            warningLine_[i]->SetPosition(warningLinePos_[i]);
            warningLine_[i]->Update();
        }

        // 警告スプライトの点滅
        easing->Color(warningEasing_, 0.05f, 0);
        if (warningEasing_.colorTime >= 1.0f) {
            warningEasing_.colorTime = 0.0f;

            if (warningEasing_.startColor.w == 0.0f) {
                warningEasing_.startColor.w = 1.0f;
                warningEasing_.endColor.w = 0.0f;
            } else {
                warningEasing_.startColor.w = 0.0f;
                warningEasing_.endColor.w = 1.0f;
            }
        }

        // フェード
        if (warningTimer_ <= 0.5f) {
            intensity = (std::max)(0.0f, intensity - 1.0f / 30.0f);

            // SEフェードアウト
            seVolume_ = (std::max)(0.0f, seVolume_ - 1.0f / 30.0f); // 音量も同じ速度で減少
            SoundManager::GetInstance()->SetVolume("warning_se", seVolume_);

        }

        warning_->SetColor(warningEasing_.color);
        warning_->Update();

        // SE
        if (isSePlayed_) {
            SoundManager::GetInstance()->Play("warning_se", true, seVolume_);
			isSePlayed_ = false;
        }

        // BGMのフェードアウト処理
        bgmVolume_ = (std::max)(0.0f, bgmVolume_ - 1.0f / 30.0f); // 音量も同じ速度で減少
        SoundManager::GetInstance()->SetVolume("stage.mp3", bgmVolume_);
        if (bgmVolume_ <= 0.0f)
            SoundManager::GetInstance()->Stop("stage.mp3");

    } else {
        isWarning_ = false;
        isBossAppears_ = true;
    }
}

void GamePlayScene::BossAppearsUpdate()
{

    bossAppearsTimer_ = (std::max)(0.0f, bossAppearsTimer_ - 1.0f / 60.0f);

    if (bossAppearsState_ == Grapes) {
        if (bossAppearsTimer_ > 0.0f) {
            if (bossAppearsTimer_ > 0.5f) {
                if (intensity >= 1.0f) {
                    for (int i = 0; i < kBossAppearsGrapes_; i++) {
                        if (grapesEasing_[i - 1].moveTime >= 0.5f || i == 0) {
                            easing->Move(grapesEasing_[i], 0.03f, 0);

                            bossAppearsGrapes_[i]->SetTranslate(grapesEasing_[i].transform.translate);
                            bossAppearsGrapes_[i]->Update();
                        }
                    }

                    if (grapesEasing_[0].moveTime >= 1.0f) {
                        // ボス登場の文字
                        easing->SizeV2(nameEasing_, 0.05f, 0);
                        bossAppearsName_[0]->SetSize(nameEasing_.size);
                        bossAppearsName_[0]->Update();
                    }

                    if (grapesEasing_[6].moveTime >= 1.0f) {
                        // 回転アニメーション
                        if (grapesEasing_[6].rotationTime >= 1.0f) {

                            // 【解決策2の組み込み】
                            // static を付けることで、ゲーム起動時に1回だけ初期化され、ランダムな状態が保持されます
                            static std::random_device seed_gen;
                            static std::mt19937 engine(seed_gen());

                            // 1. ループの前に、今回「逆回転」させるぶどうを1つだけランダムに選ぶ
                            if (kBossAppearsGrapes_ > 1) {
                                // 1 から (kBossAppearsGrapes_ - 1) の範囲で均等に割り振る設定
                                std::uniform_int_distribution<int> dist(1, kBossAppearsGrapes_ - 1);

                                int nextRandGrapes = randGrapes_;
                                while (nextRandGrapes == randGrapes_) { // 前回と同じぶどうが選ばれないようにする
                                    nextRandGrapes = dist(engine); // メルセンヌ・ツイスタで乱数を生成
                                }
                                randGrapes_ = nextRandGrapes;
                            }

                            // 2. 全体の回転方向を決定・リセットするループ
                            for (int i = 1; i < kBossAppearsGrapes_; i++) {
                                if (i == randGrapes_) {
                                    // ランダムに選ばれたぶどうは逆回転（3.14 -> 0.0）
                                    grapesEasing_[i].startRotation.y = 3.14f;
                                    grapesEasing_[i].endRotation.y = 0.0f;
                                } else {
                                    // それ以外のぶどう
                                    if (grapesEasing_[i].transform.rotate.y == 0.0f) {
                                        grapesEasing_[i].startRotation.y = 0.0f;
                                        grapesEasing_[i].endRotation.y = 3.14f;
                                    } else {
                                        grapesEasing_[i].startRotation.y = 3.14f;
                                        grapesEasing_[i].endRotation.y = 3.14f;
                                    }
                                }

                                // イージング時間のリセット
                                grapesEasing_[i].rotationTime = 0.0f;
                                grapesEasing_[i].rotationEasedT = 0.0f;
                            }
                        }

                        // --- 毎フレームの更新処理 ---
                        for (int i = 1; i < kBossAppearsGrapes_; i++) {
                            easing->Rotation(grapesEasing_[i], 0.03f, 0);
                            bossAppearsGrapes_[i]->SetRotate(grapesEasing_[i].transform.rotate);
                            bossAppearsGrapes_[i]->Update();
                        }

                        // 放射線ブラー
                        isRadialBlur = true;
                        if (blurWidth < 0.01f)
                            blurWidth += 0.001f;
                        // 集中線
                        isConcentrationLines = true;
                        concentrationLineIntensity = 0.02f; // 線の濃さ
                        concentrationLineLength = 0.35f; // 線の長さ（中心からの開始距離 0.0〜1.0）
                    }

                } else {
                    // フェードイン
                    intensity += 1.0f / 30.0f;
                }
            } else {
                // フェードアウト
                intensity = (std::max)(0.0f, intensity - 1.0f / 30.0f);

                if (intensity <= 0.0f) {
                    // 放射線ブラー
                    isRadialBlur = false;
                    // 集中線
                    isConcentrationLines = false;
                }
            }

            // 集中線
            PostEffect::GetInstance()->SetConcentrationLines(isConcentrationLines);
            PostEffect::GetInstance()->SetConcentrationLineIntensity(concentrationLineIntensity);
            PostEffect::GetInstance()->SetConcentrationLineCenter(concentrationLineCenter);
            PostEffect::GetInstance()->SetConcentrationLineDensity(concentrationLineDensity);
            PostEffect::GetInstance()->SetConcentrationLineLength(concentrationLineLength);
            PostEffect::GetInstance()->SetConcentrationLineSpeed(concentrationLineSpeed);

        } else {
            isBossAppears_ = false;
        }
    } else if (bossAppearsState_ == Banana) {
        if (bossAppearsTimer_ > 0.0f) {
            if (bossAppearsTimer_ > 0.5f) {
                if (intensity >= 1.0f) {

                    easing->Move(bananaEasing_[0], 0.01f, 0);
                    bossAppearsBanana_[0]->SetTranslate(bananaEasing_[0].transform.translate);
                    bossAppearsBanana_[0]->Update();

                    if (bananaEasing_[0].moveTime >= 1.0f) {
                        for (int i = 1; i < kBossAppearsBanana_; i++) {
                            if (bananaEasing_[i - 1].moveTime >= 0.5f || i == 1) {
                                easing->Move(bananaEasing_[i], 0.03f, 0);

                                bossAppearsBanana_[i]->SetTranslate(bananaEasing_[i].transform.translate);
                                bossAppearsBanana_[i]->Update();
                            }
                        }

                        // ボス登場の文字
                        easing->SizeV2(nameEasing_, 0.05f, 0);
                        bossAppearsName_[1]->SetSize(nameEasing_.size);
                        bossAppearsName_[1]->Update();
                    }

                    if (bossAppearsTimer_ <= 2.0f) {
                        // 放射線ブラー
                        isRadialBlur = true;
                        if (blurWidth < 0.01f)
                            blurWidth += 0.001f;
                        // 集中線
                        isConcentrationLines = true;
                        concentrationLineIntensity = 0.02f; // 線の濃さ
                        concentrationLineLength = 0.35f; // 線の長さ（中心からの開始距離 0.0〜1.0）
                    }

                } else {
                    // フェードイン
                    intensity += 1.0f / 30.0f;
                }
            } else {
                // フェードアウト
                intensity = (std::max)(0.0f, intensity - 1.0f / 30.0f);

                if (intensity <= 0.0f) {
                    // 放射線ブラー
                    isRadialBlur = false;
                    // 集中線
                    isConcentrationLines = false;
                }
            }

            // 集中線
            PostEffect::GetInstance()->SetConcentrationLines(isConcentrationLines);
            PostEffect::GetInstance()->SetConcentrationLineIntensity(concentrationLineIntensity);
            PostEffect::GetInstance()->SetConcentrationLineCenter(concentrationLineCenter);
            PostEffect::GetInstance()->SetConcentrationLineDensity(concentrationLineDensity);
            PostEffect::GetInstance()->SetConcentrationLineLength(concentrationLineLength);
            PostEffect::GetInstance()->SetConcentrationLineSpeed(concentrationLineSpeed);

        } else {
            isBossAppears_ = false;
        }
    }
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
