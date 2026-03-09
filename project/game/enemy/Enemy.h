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

class Enemy {
public:
    /// <summary>
    /// èâä˙âª
    /// </summary>
    /// <param name="camera"></param>
    virtual void Initialize(Camera* camera, Vector3 pos);

    /// <summary>
    /// çXêV
    /// </summary>
    virtual void Update();

    /// <summary>
    /// ï`âÊ
    /// </summary>
    virtual void Draw3D();

    /* Getä÷êî */
    virtual Vector3 GetWorldPosition() const = 0;
    virtual float GetRadius() const = 0;
    // virtual void OnCollision() = 0;
    const std::vector<std::unique_ptr<EnemyBullet>>& GetBullets() const { return enemyBullet_; }

    /* Setä÷êî */
    virtual void SetTargetPlayer(Player* target) { };

protected:
    // íe
    std::vector<std::unique_ptr<EnemyBullet>> enemyBullet_;

private:
};
