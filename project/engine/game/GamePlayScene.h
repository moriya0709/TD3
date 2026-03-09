#pragma once
#include "BaseScene.h"
#include "Camera.h"
#include "CameraController.h"
#include "CameraManager.h"
#include "ImGuiManager.h"
#include "Input.h"
#include "ModelManager.h"
#include "Object.h"
#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include "PostEffect.h"
#include "SoundManager.h"
#include "Sprite.h"

#include "../../game/enemy/EnemyManager.h"
#include "../../game/player/Player.h"

class SpriteCommon;
class ObjectCommon;

class GamePlayScene : public BaseScene {
public:
    // 初期化
    void Initialize() override;
    // 更新
    void Update() override;
    // 描画
    void Draw2D() override;
    void Draw3D() override;
    // 終了
    void Finalize() override;

private:
    Transform cameraTransform {
        { 1.0f, 1.0f, 1.0f }, // scale
        { 0.0f, 0.0f, 0.0f }, // rotate
        { 0.0f, 1.0f, -5.0f } // translate
    };
    // パーティクル
    Transform transformParticle {
        { 1.0f, 1.0f, 1.0f },
        { 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f }
    };

    // レティクル
    Vector2 reticle = { 0.0f };

    // *ライティング* //

    // 平行光
    bool isDirectionalLight = false;
    Vector4 DirectionalLightColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    Vector3 DirectionalLightDirection = { 0.0f, -1.0f, 0.0f };
    float DirectionalLightIntensity = 1.0f;
    // 環境光
    bool isAmbientLight = true;
    Vector4 AmbientLightColor = { 0.2f, 0.2f, 0.2f };
    float AmbientLightIntensity = 1.0f;
    // ポイントライト
    bool isPointLight = false;
    Vector4 PointLightColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    Vector3 PointLightPosition = { 1.0f, 1.0f, 0.0f };
    float PointLightIntensity = 1.0f;
    // スポットライト
    bool isSpotLight = false;
    Vector4 SpotLightColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    Vector3 SpotLightPosition = { 0.0f, 0.0f, 0.0f };
    Vector3 SpotLightDirection = { 0.0f, 0.0f, 0.0f };
    float SpotLightRange = 10.0f;
    float SpotLightIntensity = 1.0f;

    // *ポストエフェクト* //

    // 反転
    bool isInversion = false;
    // グレースケール
    bool isGrayscale = false;

    // 放射線ブラー
    bool isRadialBlur = false;
    Vector2 blurCenter = { 0.5f, 0.5f };
    float blurWidth = 0.01f;
    int blurSamples = 10;

    // ディスタンスフォグ
    bool isDistanceFog = false;
    Vector3 distanceFogColor = { 0.5f, 0.5f, 0.5f };
    float distanceStart = 5.0f;
    float distanceEnd = 20.0f;

    // ハイトフォグ
    bool isHeightFog = false;
    Vector3 heightFogColor = { 0.5f, 0.5f, 0.5f };
    float heightFogTop = 0.0f;
    float heightFogBottom = -5.0f;
    float heightFogDensity = 1.0f;

    // DOF
    bool isDOF = false;
    float focusDistance = 5.0f;
    float focusRange = 5.0f;
    float bokehRadius = 5.0f;

    // カメラ
    std::unique_ptr<Camera> camera = nullptr;
    // スプライト
    std::unique_ptr<Sprite> sprite = nullptr;
    // 3Dオブジェクト
    std::unique_ptr<Object> object {};
    // パーティクルエミッタ
    std::unique_ptr<ParticleEmitter> particleEmitter = nullptr;

    std::unique_ptr<Player> player_ = nullptr;
    // リスト
    std::unique_ptr<EnemyManager> enemy_ = nullptr;
    std::unique_ptr<CameraController> CameraController_ = nullptr;
};