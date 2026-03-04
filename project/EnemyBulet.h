#pragma once
#include "BaseScene.h"
#include "Camera.h"
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

class EnemyBulet {
public:
    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="camera"></param>
    virtual void Initialize(Camera* camera);

    /// <summary>
    /// 更新
    /// </summary>
    virtual void Update();

    /// <summary>
    /// 描画
    /// </summary>
    virtual void Draw();

private:
    Transform transform_; // 座標系
    float isAvile; // 生存しているか
    float acceleration; // 弾の速さ(個別で設定)
    float vector; // ベクトル(速さ)

    Camera* camera_ = nullptr; // カメラ
};