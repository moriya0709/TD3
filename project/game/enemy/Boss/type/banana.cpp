#define NOMINMAX

#include "banana.h"
#include "../../Normal/Bullet/HomingEnemyBullet.h"
#include "../../Normal/Bullet/TargetEnemyBullet.h"
#include "Player.h"

void banana::Initialize(Camera* camera, Vector3 pos, int health)
{
    camera_ = camera;

    health_ = health;

    Vector3 cameraPos = camera_->GetTranslate();

    baseTransform_.scale = { 14.0f, 14.0f, 14.0f };
    baseTransform_.rotate = { 0.0f, 0.0f, 0.0f };
    baseTransform_.translate = pos;

    BehaviorchangeTimer = kBehaviorchangeTimer;
    interval = maxInterval;

    isAvile = true;

    stemobject = std::make_unique<Object>();
    stemobject->Initialize(camera_);
    stemobject->SetModel("bossGrapesBranch.obj");
    stemobject->SetScale(baseTransform_.scale);
    stemobject->SetRotate(baseTransform_.rotate);
    stemobject->SetTranslate(baseTransform_.translate + cameraPos + baseStem);

    parts_.clear();

    // 時計回りに設置
    int pats = 4;

    for (int i = 0; i < pats; ++i) {
        BossPart p;

        // モデル生成
        p.object = std::make_unique<Object>();
        p.object->Initialize(camera_);
        p.object->SetModel("bossGrapesOnly.obj");
        p.object->SetScale(baseTransform_.scale);
        p.object->SetRotate(p.transform.rotate);
        p.object->SetTranslate(baseTransform_.translate + p.transform.translate + cameraPos);
        // object->SetModel(texture); テクスチャを直で変えられるコードが今はない
        p.object->Update();

        parts_.push_back(std::move(p)); // unique_ptrを含むので std::move
    }
}
