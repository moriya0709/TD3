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

#include "EnemyBullet.h"

class Player;

struct WayPoint {
    Vector3 target; // 目標の座標
    float timeToReach; // 到達にかける時間
    float timeToStop; // 停止時間
};

class Enemy {
public:
    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="camera"></param>
    virtual void Initialize(Camera* camera, Vector3 pos, int health);

    /// <summary>
    /// 更新
    /// </summary>
    virtual void Update();

    /// <summary>
    /// 描画
    /// </summary>
    virtual void Draw3D();

    /* Get関数 */
    virtual Vector3 GetWorldPosition() const = 0;
    virtual float GetRadius() const = 0;
    const std::vector<std::unique_ptr<EnemyBullet>>& GetBullets() const { return enemyBullet_; }
    virtual bool GetIsDead() const = 0;

    /* Set関数 */
    virtual void SetTargetPlayer(Player* target) { };
    virtual void OnCollision(int Damage, [[maybe_unused]] Vector3 bulletPos, [[maybe_unused]] Vector3 Velocity) = 0;
    virtual void SetWayPoints(const std::vector<WayPoint>& waypoints) { };
    virtual void SetFleeWaypoint(const WayPoint& fleeWP, bool hasFleeData) { };

protected:
    // 弾
    std::vector<std::unique_ptr<EnemyBullet>> enemyBullet_;

private:
};
