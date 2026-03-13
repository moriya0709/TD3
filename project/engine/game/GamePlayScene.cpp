#include "GamePlayScene.h"
#include "ObjectCommon.h"
#include "SceneManager.h"
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

    CameraController_ = std::make_unique<CameraController>();
    CameraController_->Initialize(camera.get());

    player_ = std::make_unique<Player>();
	player_->Initialize(camera.get(), Player::Style::normal);

    enemy_ = std::make_unique<EnemyManager>();
    enemy_->Initialize(player_.get(), camera.get(), CameraController_.get());

    // Emitパーティクル発生
    particleEmitter = std::make_unique<ParticleEmitter>();
    particleEmitter->Initialize("group1", transformParticle, 5, 1.0f);
    particleEmitter->Emit();



}

void GamePlayScene::Update()
{
    CameraController_->Update();

    // プレイヤー更新
    player_->Update(enemy_->GetEnemies());

    // 敵更新
    enemy_->SetcurrentTimer_(CameraController_->GetCurrentReplayTime());
    enemy_->Update();

    // 当たり判定
    ChekeAllCollision();

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
    PostEffect::GetInstance()->HightFogUpdate(camera.get());
    // DOF
    PostEffect::GetInstance()->SetDOF(isDOF);
    PostEffect::GetInstance()->SetFocusDistance(focusDistance);
    PostEffect::GetInstance()->SetBokehRadius(bokehRadius);
    PostEffect::GetInstance()->SetFocusRange(focusRange);

#pragma endregion

#pragma region レイマーチング
    // レイマーチング
    RayMarching::GetInstance()->SetCamera(camera.get());
    // レイマーチング
    RayMarching::GetInstance()->CameraUpdate(camera.get());
    //rayMarching->SetTime(rayMarchingTime);
    RayMarching::GetInstance()->SetSunDir(rayMarchingSunDir);
    RayMarching::GetInstance()->SetDensity(rayMarchingDensity);
    RayMarching::GetInstance()->SetCloudTop(rayMarchingCloudBottom);
    RayMarching::GetInstance()->SetCloudBottom(rayMarchingCloudTop);
    RayMarching::GetInstance()->SetRialLight(rayMarchingIsRialLight);
    RayMarching::GetInstance()->SetAnimeLight(rayMarchingIsAnimeLight);

#pragma endregion

#ifdef USE_IMGUI
    // ImGui
    // フレームレートの取得と表示
    float fps = ImGui::GetIO().Framerate;
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / fps, fps);

    // カメラ
    ImGui::DragFloat3("cameraTranslate", &cameraTransform.translate.x, 0.01f, -100.0f, 100.0f);
    ImGui::DragFloat3("cameraRotate", &cameraTransform.rotate.x, 0.01f, -180.0f, 180.0f);
    camera->SetTranslate({ cameraTransform.translate });
    camera->SetRotate({ cameraTransform.rotate });

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

    CameraController_->DrawImGui();
    enemy_->DrawImGui();

#pragma endregion

#pragma region レイマーチング
    // レイマーチング
    //ImGui::DragFloat("rayMarchingTime", &rayMarchingTime, 0.1f,0.0f,10.0f);
    ImGui::DragFloat3("rayMarchingSunDir", &rayMarchingSunDir.x, 0.1f, -50.0f, 50.0f);
    ImGui::DragFloat("rayMarchingDensity", &rayMarchingDensity, 0.01f, -5.0f, 10.0f);
    ImGui::DragFloat("rayMarchingCloudBottom", &rayMarchingCloudBottom, 10.0f, -5000.0f, 5000.0f);
    ImGui::DragFloat("rayMarchingCloudTop", &rayMarchingCloudTop, 10.0f, -5000.0f, 5000.0f);
    ImGui::Checkbox("rayMarchingIsRialLight", &rayMarchingIsRialLight);
    ImGui::Checkbox("rayMarchingIsAnimeLight", &rayMarchingIsAnimeLight);

#pragma endregion

#endif
}

void GamePlayScene::Draw2D()
{
    // 2Dオブジェクトの描画準備
    SpriteCommon::GetInstance()->SetCommonPipelineState();

    player_->Draw2D();

    // スプライト描画
    // sprite->Draw();
}

void GamePlayScene::Draw3D()
{
    // 3Dオブジェクトの描画準備
    ObjectCommon::GetInstance()->SetCommonPipelineState();
    // 3Dオブジェクト描画
    player_->Draw3D();

    enemy_->Draw3D();

    // パーティクル描画
    ParticleManager::GetInstance()->Draw();

    // アウトライン描画準備
    ObjectCommon::GetInstance()->SetOutlinePipelineState();

    // アウトライン描画
    // object->Draw();

}

void GamePlayScene::Finalize() { CameraManager::GetInstance()->RemoveCamera("main"); }

void GamePlayScene::ChekeAllCollision()
{
    const std::list<std::shared_ptr<Enemy>>& enemies = enemy_->GetEnemies();
    CheckCollisionPlayerEnemy(player_.get(), enemies);
    CheckCollisionPlayerEnemyBullet(player_.get(), enemies);
    CheckCollisionPlayerBulletEnemy(player_.get(), enemies);
}
