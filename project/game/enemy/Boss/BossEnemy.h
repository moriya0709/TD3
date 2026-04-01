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

#include "../EnemyBullet.h"

class Player;
class PlayerBullet;

struct CollisionVolume {
    Vector3 position;
    float radius;
    uint32_t partId;
};

class BossEnemy {
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
    // --- 共通の当たり判定インターフェース ---
    virtual std::vector<CollisionVolume> GetCollisionVolumes() = 0;

    // 弾を受け取り、弾を消滅させるなら true を返す
    virtual bool OnCollision(const CollisionVolume& volume, PlayerBullet* bullet) = 0;

    virtual bool GetIsDead() const = 0;

    /* Set関数 */
    virtual void SetTargetPlayer(Player* target) { };
    //virtual void OnCollision(int Damage, [[maybe_unused]] Vector3 bulletPos, [[maybe_unused]] Vector3 Velocity) = 0;
    // virtual void SetWayPoints(const std::vector<WayPoint>& waypoints) { };
    // virtual void SetFleeWaypoint(const WayPoint& fleeWP, bool hasFleeData) { };

protected:
    // 弾
    std::vector<std::unique_ptr<EnemyBullet>> enemyBullet_;

private:
};
