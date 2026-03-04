#pragma once
#include "Enemy.h"
#include "EnemyBullet.h"

class NormalEnemy : public Enemy {
public:
    void Initialize(Camera* camera) override;

    void Update() override;

    void Draw3D() override;

    // Get
    EnemyBullet* GetEnemyBullet(int number) { return enemyBullet_[number].get(); }

private:
    Transform transform_; // À•WŒn
    float activeTimer; // ‘¶İ‚·‚éŠÔ
    float isAvile; // ¶‘¶‚µ‚Ä‚¢‚é‚©
    float health; // ‘Ì—Í
    float interval; // ’e‚ğ”­Ë‚·‚éŠÔŠu
    static inline const float maxInterval = 2.0f; // ŠÔŠu
    

    std::unique_ptr<Object> object_; // ƒIƒuƒWƒF

    Camera* camera_ = nullptr; // ƒJƒƒ‰\

    // ’e
    std::vector<std::unique_ptr<EnemyBullet>> enemyBullet_;
};
