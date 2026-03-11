#include "NormalEnemy.h"
#include "../Bullet/NormalEnemyBullet.h"

void NormalEnemy::Initialize(Camera* camera, Vector3 pos, int health)
{
    camera_ = camera;

    transform_.scale = { 1.0f, 1.0f, 1.0f };
    transform_.rotate = { 0.0f, 0.0f, 0.0f };
    transform_.translate = pos;

    object_ = std::make_unique<Object>();
    object_->Initialize(camera_);
    object_->SetModel("player.obj");
    object_->SetScale(transform_.scale);
    object_->SetRotate(transform_.rotate);
    object_->SetTranslate(transform_.translate);

    health_ = health;
    isAvile = true;

    interval = maxInterval;
}

void NormalEnemy::Update()
{
    // 移動
    // transform_.translate.x += kwalkSpeed;

    // 生きていないならやられモーション処理を入れる
    if (!isAvile) {
        isDead_ = true;
    }

    // オブジェクトのセット
    object_->SetTranslate(transform_.translate);

    // 弾を生成する時間を減らす
    interval -= 1.0f / 60.0f;

    if (interval <= 0.0f) {
        // 弾の生成
        std::unique_ptr<NormalEnemyBullet> newBulletEnemy = std::make_unique<NormalEnemyBullet>();
        newBulletEnemy->Initialize(camera_, transform_.translate);
        newBulletEnemy->SetBulletAcceleration(Vector3(0.0f, 0.0f, -0.1f));

        enemyBullet_.push_back(std::move(newBulletEnemy));
        interval = maxInterval;
    }
    // 更新処理
    for (auto& bullet : enemyBullet_) {
        bullet->Update();
    }

    // 弾の削除
    std::erase_if(enemyBullet_, [](const std::unique_ptr<EnemyBullet>& bullet) {
        return !bullet->GetIsActive(); // GetIsActive が false なら削除
    });

    // ここにIMGUI

    object_->Update();
}

void NormalEnemy::Draw3D()
{
    // 3Dオブジェクト描画
    object_->Draw();

    // 更新処理
    for (auto& bullet : enemyBullet_) {
        bullet->Draw3D();
    }
}

void NormalEnemy::OnCollision(int Damage)
{
    health_ -= Damage;

    if (health_ <= 0) {
        isAvile = false;
    }
}
