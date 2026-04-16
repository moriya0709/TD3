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

void banana::Update()
{
    if (behaviorRequest_ != Behavior::kUnknown) {
        behavior_ = behaviorRequest_;

        switch (behavior_) {
        case banana::Behavior::kDefeated:
        default:

            break;
        }

        behaviorRequest_ = Behavior::kUnknown;
    }

    switch (behavior_) {
    case banana::Behavior::kAppearance:

        break;
    case banana::Behavior::kStillness:
        BehaviorStillness();
        break;
    case banana::Behavior::kAttack:
        BehaviorAttack();
        break;
    case banana::Behavior::kShield:
        BehaviorShield();
        break;
    case banana::Behavior::kDefeated:
        BehaviorDefeated();
        break;
    }

    // 発射
    BulletUpdate();

    Vector3 cameraPos = camera_->GetTranslate();

    stemobject->SetTranslate(baseTransform_.translate + cameraPos + baseStem);
    stemobject->Update();
    for (auto& part : parts_) {
        Vector3 worldPos = part.transform.translate + baseTransform_.translate + cameraPos;
        part.object->SetTranslate(worldPos);
        part.object->Update();
    }
}

void banana::Draw3D()
{
    for (auto& part : parts_) {
        part.object->Draw();
    }

    // 更新処理
    for (auto& bullet : enemyBullet_) {
        bullet->Draw3D();
    }
}

void banana::BulletMirror(const CollisionVolume& volume, PlayerBullet* bullet)
{

    // ==========================================
    // 【ダミーに当たった場合：反射弾を新規生成】
    // ==========================================

    Vector3 bulletPos = bullet->GetPosition();
    Vector3 bulletVelocity = bullet->GetVelocity();
    Vector3 enemyPos = volume.position; // 当たったパーツの座標を起点にする

    // ベクトル
    Vector3 normal = {
        bulletPos.x - enemyPos.x,
        bulletPos.y - enemyPos.y,
        bulletPos.z - enemyPos.z
    };

    // 正規化
    normal.x *= 0.01f;
    normal.y *= 0.01f;
    normal.z *= 1.0f;

    normal = Normalize(normal);

    // 反射ベクトルの計算
    float dot = bulletVelocity.x * normal.x + bulletVelocity.y * normal.y + bulletVelocity.z * normal.z;

    Vector3 reflectVelocity;
    reflectVelocity.x = bulletVelocity.x - 2.0f * dot * normal.x;
    reflectVelocity.y = bulletVelocity.y - 2.0f * dot * normal.y;
    reflectVelocity.z = bulletVelocity.z - 2.0f * dot * normal.z;

    reflectVelocity.z *= 1.3f;

    // 敵の弾（反射弾）を新しく追加
    std::unique_ptr<HomingEnemyBullet> newBulletEnemy = std::make_unique<HomingEnemyBullet>();
    // 弾の発生位置は「当たったダミーパーツの座標」にする
    newBulletEnemy->Initialize(camera_, bulletPos);
    newBulletEnemy->SetBulletAcceleration(reflectVelocity);
    newBulletEnemy->SetTargetPosition(player_->GetPosition());
    newBulletEnemy->SetUpgrade(1.0f);
    newBulletEnemy->Update();

    // GrapesBossクラスが持つ enemyBullet_ リストに追加
    enemyBullet_.push_back(std::move(newBulletEnemy));
}

Vector3 banana::GetWorldPosition() const
{
    return baseTransform_.translate;
}

float banana::GetRadius() const
{
    return 2.0f;
}

bool banana::GetIsDead() const
{
    return isDead_;
}

std::vector<CollisionVolume> banana::GetCollisionVolumes()
{
    std::vector<CollisionVolume> volumes;

    // ボスの現在の中心座標 (GrapesBoss が持っている Transform)
    Vector3 bossPos = baseTransform_.translate;

    for (uint32_t i = 0; i < parts_.size(); ++i) {

        // 1. パーツのワールド座標を計算
        // ボスの中心座標に、パーツごとの配置オフセット（ローカル座標）を足す
        Vector3 cameraPos = camera_->GetTranslate();

        Vector3 partWorldPos = {
            parts_[i].transform.translate.x + bossPos.x + cameraPos.x,
            parts_[i].transform.translate.y + bossPos.y + cameraPos.y,
            parts_[i].transform.translate.z + bossPos.z + cameraPos.z
        };

        // 2. 判定用のボリューム構造体を作成
        CollisionVolume volume;
        volume.position = partWorldPos; // 計算したワールド座標
        volume.radius = parts_[i].radius; // パーツ個別の当たり判定サイズ
        volume.partId = i; // 何番目のパーツか（OnHitで使う）

        // 3. リストに追加
        volumes.push_back(volume);
    }

    return volumes;
}

void banana::SetTargetPlayer(Player* target)
{
    player_ = target;
}

bool banana::OnCollision(const CollisionVolume& volume, PlayerBullet* bullet)
{
    uint32_t id = volume.partId;

    if (id >= parts_.size()) {
        return false;
    }

    BossPart& hitPart = parts_[id];

    if (hitPart.isWeakPoint) {
        // ==========================================
        // 【弱点（本体）に当たった場合：共有HPにダメージ】
        // ==========================================

        // ボス全体の共有体力を減らす
        health_ -= bullet->GetDamage();

        // 共有体力が0以下になったらボス撃破
        if (health_ <= 0) {
            health_ = 0;
            isDead_ = true;
        }

        return true; // プレイヤーの弾を消滅させる

    } else {
        BulletMirror(volume, bullet);
        // ★ プレイヤーの弾自体はここで「消滅」させるため true を返す
        return true;
    }
}

void banana::BulletUpdate()
{
    // 弾を生成する時間を減らす
    if (behavior_ == Behavior::kStillness) {
        interval -= 1.0f / 60.0f;
    }

    if (interval <= 0.0f) {
        // 弾の生成
        std::unique_ptr<TargetEnemyBullet> newBulletEnemy = std::make_unique<TargetEnemyBullet>();

        for (auto& part : parts_) {
            if (!part.isWeakPoint)
                continue;

            newBulletEnemy->Initialize(camera_, part.transform.translate + baseTransform_.translate + camera_->GetTranslate());
            break;
        }

        // 弱点が攻撃をする
        newBulletEnemy->SetBulletAcceleration(Vector3(0.0f, 0.0f, -0.08f));
        newBulletEnemy->SetTargetPosition(player_->GetPosition());
        newBulletEnemy->SetAcceleration(0.5f);
        newBulletEnemy->SetUpgrade(1.0f);
        newBulletEnemy->Update();

        intervalCount++;
        if (intervalCount < 3) {
            interval = 0.2f;
        } else {
            interval = maxInterval;
            intervalCount = 0;
        }
        enemyBullet_.push_back(std::move(newBulletEnemy));
    }

    // 更新処理
    for (auto& bullet : enemyBullet_) {
        bullet->SetTargetPositionUpdate(player_->GetPosition());
        bullet->Update();
    }

    // 弾の削除
    std::erase_if(enemyBullet_, [](const std::unique_ptr<EnemyBullet>& bullet) {
        return !bullet->GetIsActive(); // GetIsActive が false なら削除
    });
}

void banana::BehaviorStillness()
{
}

void banana::BehaviorAttack()
{
}

void banana::BehaviorShield()
{
}

void banana::BehaviorDefeated()
{
}
