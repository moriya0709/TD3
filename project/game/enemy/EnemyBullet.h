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

class EnemyBullet {
public:
    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="camera">カメラ</param>
    /// <param name="Pos">初期座標</param>
    virtual void Initialize(Camera* camera, Vector3 Pos);

    /// <summary>
    /// 更新
    /// </summary>
    virtual void Update();

    /// <summary>
    /// 描画
    /// </summary>
    virtual void Draw3D();

    /* Get関数 */
    // 生存確認用の仮想関数を追加
    virtual bool GetIsActive() const = 0;
    virtual Vector3 GetWorldPosition() const = 0;
    virtual float GetRadius() const = 0;

    /* Set関数 */
    // 座標をセットする関数
    virtual void SetPosition(Vector3 Pos) = 0;
    virtual void SetBulletAcceleration(Vector3 num) = 0;
    virtual void SetactiveTimer(float num) = 0;
    virtual void SetTargetPosition(Vector3 Pos) = 0;
    virtual void OnCollision() = 0;

private:
    // Transform transform_; // 座標系
    // float isAvile; // 生存しているか
    // float acceleration; // 弾の速さ(個別で設定)
    // float vector; // ベクトル(速さ)

    // Camera* camera_ = nullptr; // カメラ
};