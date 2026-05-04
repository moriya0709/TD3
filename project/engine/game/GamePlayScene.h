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
#include "ScoreManager.h"
#include"../game/collision/Collision.h"

#include "../../game/enemy/EnemyManager.h"
#include "../../game/player/Player.h"

#include "RayMarching.h"
#include "Easing.h"

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
    void SetPlayerStyle(int style) override;
	void SetCurrentStage(int currentStage) override;


    void ChekeAllCollision() ;

    void PauseSelect();

    //ステージクリア
    void StageClear();

    //ポーズ選択
    enum Pause { kResume, kRetry, kSelect };

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
    bool isRadialBlur = true;
    Vector2 blurCenter = { 0.5f, 0.5f };
    float blurWidth = 0.05f;
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

    // ブルーム
    float bloomThreshold = 1.5f;
    float bloomIntensity = 1.0f;
    float bloomBlurRadius = 1.0f;

    // レンズフレア
    bool isLensFlare = true;           // レンズフレアのON/OFF
    int lensFlareGhostCount = 6;   // ゴーストの数（例: 4～8）
    float lensFlareHaloWidth = 0.57f;      // ヘイロー（輪っか）の大きさ
    bool isACES = true;                 // ACESトーンマッピングをONにする
    float caIntensity = 0.05f;          // 色収差の強さ（最初は弱めに）

    // モーションブラー
    bool isMotionBlur = true;    // モーションブラーのON/OFF
    int motionBlurSamples = 16; // サンプル数（例：8〜16）
    float motionBlurScale = 1.0f;   // ブラーの強さ
    // 色収差
    bool isFullScreenCA = true; // 画面全体の色収差ON/OFF
    float fullScreenCAIntensity = 1.0f; // 画面全体の色収差の強さ
	// スピードディストーション
	bool isSpeedDistortion = true; // スピードディストーションのON/OFF
    float speedDistortionStrength = 1.0f; // 歪みの強さ

    // レイマーチング
    //float rayMarchingTime = 0.0f; ;
    Vector3 rayMarchingSunDir = { 0.07f, -0.17f, -0.75f };
    float rayMarchingCloudCoverage = 0.22f;
    float rayMarchingCloudBottom = 70.0f;
    float rayMarchingCloudTop = -300.0f;
    bool rayMarchingIsRialLight = false;
    bool rayMarchingIsAnimeLight = true;
    bool  rayMarchingIsMotionBlur = false;
    float  rayMarchingCloudOpacity = 0.01f;
    bool isStorm = false;
    float thunderFrequency = 0.3f;
    float thunderBrightness = 120.0f;

    // カメラ
    std::unique_ptr<Camera> camera = nullptr;
    // スプライト
    std::unique_ptr<Sprite> sprite = nullptr;
    // 3Dオブジェクト
    std::unique_ptr<Object> object {};

    std::unique_ptr<Player> player_ = nullptr;
    
    //スプライト
    std::unique_ptr<Sprite> pause_ = nullptr;
    std::unique_ptr<Sprite> resume_ = nullptr;
    std::unique_ptr<Sprite> retry_ = nullptr;
    std::unique_ptr<Sprite> select_ = nullptr;
    //playerHPバー
    std::unique_ptr<Sprite> playerHpUI_ = nullptr;
    std::unique_ptr<Sprite> pauseBg_ = nullptr;

    Style style_ = Style::normal;
    // リスト
    std::unique_ptr<EnemyManager> enemy_ = nullptr;
    std::unique_ptr<CameraController> cameraController_ = nullptr;
	int specialAttackTimer = 0;

    // イージング
    std::unique_ptr<Easing> easing = nullptr;
    EasingSet resumeEasing;
    EasingSet retryEasing;
    EasingSet selectEasing;

    void LithingEffect();
	void UpdateImGui();

    bool isBossBattle_ = false;

    //ポーズかどうか
    bool isPause_ = false;
	bool isPauseEasing_ = false;
    enum EasingType { Start, Select };
	EasingType easingType_ = Start;

    Pause currentPause_ = kResume;

    int currentStage_ = 0;

    //スコア保持用
    int score_ = 0;
    //今のプレイ時間
    float playTimer_ = 0.0f;
    //制限時間
    const float kMaxTime_ = 180.0f;
    //クリアしたかどうか
    bool isFinished_ = false;

    //スコア管理用
    ScoreManager scoreManager_;

    // シーン切り替わり後
	bool isSceneChanged_ = true;
    void SceneChangedEffect();

};
