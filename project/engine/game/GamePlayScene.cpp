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

    cameraContoroller_ = std::make_unique<CameraController>();
	cameraContoroller_->Initialize(camera.get());




    player_ = std::make_unique<Player>();
    player_->Initialize(camera.get());

    Enemy_ = std::make_unique<NormalEnemy>();
    Enemy_->Initialize(camera.get());

    // Emitパーティクル発生
    particleEmitter = std::make_unique<ParticleEmitter>();
    particleEmitter->Initialize("group1", transformParticle, 5, 1.0f);
    particleEmitter->Emit();

    // 初期化済みの3Dオブジェクトにモデルを紐づける
}

void GamePlayScene::Update()
{
	cameraContoroller_->Update();
    // プレイヤー更新
    player_->Update();

    // 敵更新
    Enemy_->Update();

#pragma region ライティング
    // *ライティング* //

    // 平行光
    // object->SetDirectionalLight(isDirectionalLight);
    // object->SetDirectionalLightDirection(DirectionalLightDirection);
    // object->SetDirectionalLightColor(DirectionalLightColor);
    // object->SetDirectionalLightIntensity(DirectionalLightIntensity);
    //// 環境光
    // object->SetAmbientLight(isAmbientLight);
    // object->SetAmbientLightColor(AmbientLightColor);
    // object->SetAmbientLightIntensity(AmbientLightIntensity);
    //// ポイントライト
    // object->SetPointLight(isPointLight);
    // object->SetPointLightColor(PointLightColor);
    // object->SetPointLightPosition(PointLightPosition);
    // object->SetPointLightIntensity(PointLightIntensity);
    //// スポットライト
    // object->SetSpotLight(isSpotLight);
    // object->SetSpotLightColor(SpotLightColor);
    // object->SetSpotLightPosition(SpotLightPosition);
    // object->SetSpotLightDirection(SpotLightDirection);
    // object->SetSpotLightRange(SpotLightRange);
    // object->SetSpotLightIntensity(SpotLightIntensity);
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
    PostEffect::GetInstance()->HightFogUpdate(camera.get());
    // DOF
    PostEffect::GetInstance()->SetDOF(isDOF);
    PostEffect::GetInstance()->SetFocusDistance(focusDistance);
    PostEffect::GetInstance()->SetBokehRadius(bokehRadius);
    PostEffect::GetInstance()->SetFocusRange(focusRange);

#pragma endregion

#ifdef USE_IMGUI
    // ImGui

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

    Enemy_->Draw3D();

    // パーティクル描画
    ParticleManager::GetInstance()->Draw();

    // アウトライン描画準備
    ObjectCommon::GetInstance()->SetOutlinePipelineState();

    // アウトライン描画
    // object->Draw();
}

void GamePlayScene::Finalize() { CameraManager::GetInstance()->RemoveCamera("main"); }
