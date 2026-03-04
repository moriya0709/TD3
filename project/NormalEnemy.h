#pragma once
#include "Enemy.h"

class NormalEnemy : public Enemy {
public:

    void Initialize(Camera* camera) override;

    void Update() override;

    void Draw3D() override;

private:
    Transform transform_; // 座標系
    float activeTimer; // 存在する時間
    float isAvile; // 生存しているか
    float health; // 体力

    std::unique_ptr<Object> object_; // オブジェ

    Camera* camera_ = nullptr; //カメラ―
};
