#include "grapesBoss.h"
#include "Player.h"
#include <game/enemy/Normal/Bullet/HomingEnemyBullet.h>

void grapesBoss::Initialize(Camera* camera, Vector3 pos, int health)
{
    camera_ = camera;

    health_ = health;

    Vector3 cameraPos = camera_->GetTranslate();

    baseTransform_.scale = { 5.0f, 5.0f, 5.0f };
    baseTransform_.rotate = { 0.0f, 0.0f, 0.0f };
    baseTransform_.translate = pos;

    parts_.clear();

    float spacingX = 5.0f; // パーツ間の横の間隔
    float spacingY = 5.5f; // パーツ間の縦の間隔

    // 各行のパーツ数：3体, 2体, 1体
    std::vector<int> rowCounts = { 3, 2, 1 };

    for (int rowIndex = 0; rowIndex < rowCounts.size(); ++rowIndex) {
        int countInRow = rowCounts[rowIndex];

        for (int i = 0; i < countInRow; ++i) {
            BossPart p;

            // 相対座標の計算
            float xOffset = (i - (countInRow - 1) * 0.5f) * spacingX;
            float yOffset = (1 - rowIndex) * spacingY;
            p.transform.translate = { xOffset, yOffset, 0.0f };

            // 最初は一番上の段の真ん中（index 1）を本体にする
            p.isWeakPoint = (rowIndex == 0 && i == 1);

            // モデル生成
            p.object = std::make_unique<Object>();
            p.object->Initialize(camera_);
            p.object->SetModel("bossGrapesOnly.obj");
            p.object->SetScale(baseTransform_.scale);
            p.object->SetRotate(baseTransform_.rotate);
            p.object->SetTranslate(baseTransform_.translate + p.transform.translate + cameraPos);
            // object->SetModel(texture); テクスチャを直で変えられるコードが今はない
            p.object->Update();

            parts_.push_back(std::move(p)); // unique_ptrを含むので std::move
        }
    }
    currentWeakPointIndex_ = 1; // 初期状態の本体インデックス
}

void grapesBoss::Update()
{
  /*  WeakPointchangeTimer -= 1.0f / 60.0f;
    if (WeakPointchangeTimer <= 0.0f) {
        WeakPointChange();
        WeakPointchangeTimer = kBehaviorchangeTimer;
    }*/

    if (behaviorRequest_ != Behavior::kUnknown) {
        behavior_ = behaviorRequest_;

        switch (behavior_) {
        case grapesBoss::Behavior::kDefeated:
        default:

            break;
        }

        behaviorRequest_ = Behavior::kUnknown;
    }

    switch (behavior_) {
    case grapesBoss::Behavior::kAppearance:

        break;
    case grapesBoss::Behavior::kStillness:
        BehaviorStillness();
        break;
    case grapesBoss::Behavior::kAttack:
        BehaviorAttack();
        break;
    case grapesBoss::Behavior::kShield:
        BehaviorShield();
        break;
    case grapesBoss::Behavior::kDefeated:
        BehaviorDefeated();
        break;
    }

    // 発射
    BulletUpdate();

    Vector3 cameraPos = camera_->GetTranslate();
    for (auto& part : parts_) {
        Vector3 worldPos = part.transform.translate + baseTransform_.translate + cameraPos;
        part.object->SetTranslate(worldPos);
        part.object->Update();
    }
}

void grapesBoss::Draw3D()
{
    for (auto& part : parts_) {
        part.object->Draw();
    }

    // 更新処理
    for (auto& bullet : enemyBullet_) {
        bullet->Draw3D();
    }
}

void grapesBoss::BulletMirror(const CollisionVolume& volume, PlayerBullet* bullet)
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
    newBulletEnemy->Initialize(camera_, enemyPos);
    newBulletEnemy->SetBulletAcceleration(reflectVelocity);
    newBulletEnemy->SetTargetPosition(player_->GetPosition());
    newBulletEnemy->Update();

    // GrapesBossクラスが持つ enemyBullet_ リストに追加
    enemyBullet_.push_back(std::move(newBulletEnemy));
}

void grapesBoss::WeakPointChange()
{
    if (parts_.size() < 2)
        return;

    // 座標だけを抜き出したリストを作る
    std::vector<Vector3> positions;
    for (const auto& part : parts_) {
        positions.push_back(part.transform.translate);
    }

    // 座標リストをランダムに並び替える（<algorithm>のshuffleを使用）
    static std::random_device seed_gen;
    static std::mt19937 engine(seed_gen());
    std::shuffle(positions.begin(), positions.end(), engine);

    // 並び替えた座標をパーツに再割り当て
    for (int i = 0; i < (int)parts_.size(); ++i) {
        parts_[i].transform.translate = positions[i];
    }
}

Vector3 grapesBoss::GetWorldPosition() const
{
    return baseTransform_.translate;
}

float grapesBoss::GetRadius() const
{
    return radius;
}

bool grapesBoss::GetIsDead() const
{
    return isDead_;
}

bool grapesBoss::OnHit(const CollisionVolume& volume, PlayerBullet* bullet)
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

std::vector<CollisionVolume> grapesBoss::GetCollisionVolumes()
{
    std::vector<CollisionVolume> volumes;

    // ボスの現在の中心座標 (GrapesBoss が持っている Transform)
    Vector3 bossPos = baseTransform_.translate;

    for (uint32_t i = 0; i < parts_.size(); ++i) {

        // 1. パーツのワールド座標を計算
        // ボスの中心座標に、パーツごとの配置オフセット（ローカル座標）を足す
        Vector3 partWorldPos = {
            parts_[i].transform.translate.x + bossPos.x,
            parts_[i].transform.translate.y + bossPos.y,
            parts_[i].transform.translate.z + bossPos.z
        };

        // 2. 判定用のボリューム構造体を作成
        CollisionVolume volume;
        volume.position = partWorldPos; // 計算したワールド座標
        volume.radius = radius; // パーツ個別の当たり判定サイズ
        volume.partId = i; // 何番目のパーツか（OnHitで使う）

        // 3. リストに追加
        volumes.push_back(volume);
    }

    return volumes;
}

void grapesBoss::SetTargetPlayer(Player* target)
{
    player_ = target;
}

void grapesBoss::BulletUpdate()
{
    // 弾を生成する時間を減らす
    // if (behavior_ == Behavior::kWalk) {
    //    interval -= 1.0f / 60.0f;
    //}

    // if (interval <= 0.0f) {
    //     // 弾の生成
    //     std::unique_ptr<TargetEnemyBullet> newBulletEnemy = std::make_unique<TargetEnemyBullet>();
    //     newBulletEnemy->Initialize(camera_, transform_.translate);
    //     newBulletEnemy->SetBulletAcceleration(Vector3(0.0f, 0.0f, -0.08f));
    //     newBulletEnemy->SetTargetPosition(player_->GetPosition());

    //    enemyBullet_.push_back(std::move(newBulletEnemy));
    //    interval = maxInterval;
    //}

    // 更新処理
    for (auto& bullet : enemyBullet_) {
        bullet->SetTargetPosition(player_->GetPosition());
        bullet->Update();
    }

    // 弾の削除
    std::erase_if(enemyBullet_, [](const std::unique_ptr<EnemyBullet>& bullet) {
        return !bullet->GetIsActive(); // GetIsActive が false なら削除
    });
}

void grapesBoss::OnCollision(int Damage, Vector3 bulletPos, Vector3 Velocity)
{
}

void grapesBoss::BehaviorStillness()
{
}

void grapesBoss::BehaviorAttack()
{
}

void grapesBoss::BehaviorShield()
{
}

void grapesBoss::BehaviorDefeated()
{
}
