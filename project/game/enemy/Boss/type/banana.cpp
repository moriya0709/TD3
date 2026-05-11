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

    parts_.clear();

    // 時計回りに設置
    int pats = 4;

    for (int i = 0; i < pats; ++i) {
        BossPart p;

        p.transform.rotate = { 0.0f, 0.0f, 0.0f };
        p.transform.scale = { 14.0f, 14.0f, 14.0f };
        p.transform.translate = { 0.0f, 0.0f, 0.0f };

        // モデル生成
        p.object = std::make_unique<Object>();
        p.object->Initialize(camera_);
        if (i == 0) {
            p.object->SetModel("bossBananPeelBuck.obj");
        } else if (i == 1) {
            p.object->SetModel("bossBananPeelLeft.obj");
        } else if (i == 2) {
            p.object->SetModel("bossBananPeelRight.obj");
        } else if (i == 3) {
            p.object->SetModel("bossBananBody.obj");
        }

        p.object->SetScale(p.transform.scale);
        p.object->SetRotate(p.transform.rotate);
        p.object->SetTranslate(p.transform.translate + baseTransform_.translate);
        p.object->Update();
        p.repairTimer = p.kRepairTime;
        p.PartsHp = p.kPartsHp;

        float offsetRadius = 5.0f; // 本体(0,0,0)を囲む半径。少し広めにするのがコツ

        if (i == 0) { // 皮1: 正面 (Zマイナス側をふさぐ)
            p.collisionLocalPos = { 0.0f, 0.0f, -offsetRadius };
            p.isWeakPoint = false;
        } else if (i == 1) { // 皮2: 右後ろ (120度の位置)
            p.collisionLocalPos = { offsetRadius * 0.866f, 0.0f, offsetRadius * 0.5f };
            p.isWeakPoint = false;
        } else if (i == 2) { // 皮3: 左後ろ (240度の位置)
            p.collisionLocalPos = { -offsetRadius * 0.866f, 0.0f, offsetRadius * 0.5f };
            p.isWeakPoint = false;
        } else if (i == 3) { // 本体: 中心
            p.collisionLocalPos = { 0.0f, 0.0f, 0.0f };
            p.isWeakPoint = true;
        }
        parts_.push_back(std::move(p));
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

    for (auto& part : parts_) {
        if (!part.isWeakPoint && part.PartsHp <= 0) {
            part.repairTimer -= 1.0f / 60.0f;
            if (part.repairTimer <= 0.0f) {
                part.PartsHp = BossPart::kPartsHp; // 修理完了
                part.repairTimer = BossPart::kRepairTime;
            }
        }

        // 動く場合の座標セット
        Vector3 worldPos = part.transform.translate + baseTransform_.translate;
        part.object->SetTranslate(worldPos);
        part.object->Update();
    }
}

void banana::Draw3D()
{

    for (auto& part : parts_) {
        if (part.PartsHp >= 0) {
            part.object->Draw();
        }
    }

    // 更新処理
    for (auto& bullet : enemyBullet_) {
        bullet->Draw3D();
    }
}

void banana::BulletMirror(const CollisionVolume& volume, PlayerBullet* bullet)
{
    Vector3 bulletPos = bullet->GetPosition();
    Vector3 bulletVelocity = bullet->GetVelocity();
    Vector3 normal = volume.normal;

    // ==========================================
    // 1. 反射ベクトルの計算
    // ==========================================
    float dot = bulletVelocity.x * normal.x + bulletVelocity.y * normal.y + bulletVelocity.z * normal.z;

    // 弾が内側に向かっている場合のみ反射（念のため）
    if (dot > 0)
        return;

    Vector3 reflectVelocity;
    reflectVelocity.x = bulletVelocity.x - 2.0f * dot * normal.x;
    reflectVelocity.y = bulletVelocity.y - 2.0f * dot * normal.y;
    reflectVelocity.z = bulletVelocity.z - 2.0f * dot * normal.z;

    // ゲーム的な調整：少し加速させてプレイヤーに突き返す
    float speedBoost = 1.2f;
    reflectVelocity.x *= speedBoost;
    reflectVelocity.y *= speedBoost;
    reflectVelocity.z *= speedBoost;

    // ==========================================
    // 2. 発生位置の補正（めり込み防止）
    // ==========================================
    // 矩形の表面から少し浮かせた位置に移動させる
    Vector3 spawnPos = bulletPos;
    float pushOutDist = 0.5f; // 少し外側にずらす
    spawnPos.x += normal.x * pushOutDist;
    spawnPos.y += normal.y * pushOutDist;
    spawnPos.z += normal.z * pushOutDist;

    // ==========================================
    // 3. 反射弾（ホーミング弾）の生成
    // ==========================================
    std::unique_ptr<HomingEnemyBullet> newBulletEnemy = std::make_unique<HomingEnemyBullet>();

    newBulletEnemy->Initialize(camera_, spawnPos);

    // 反射ベクトルをセット（クラスの仕様に合わせて調整してください）
    newBulletEnemy->SetBulletAcceleration(reflectVelocity);

    // プレイヤーを狙わせる
    newBulletEnemy->SetTargetPosition(player_->GetPosition());
    newBulletEnemy->SetUpgrade(1.0f);
    newBulletEnemy->Update();

    enemyBullet_.push_back(std::move(newBulletEnemy));
}

banana::CollisionVolume banana::CreateVolumeFromPart(uint32_t i, Vector3 bossPos, Vector3 cameraPos)
{
    banana::CollisionVolume volume;
    Vector3 partWorldPos = {
        parts_[i].collisionLocalPos.x + bossPos.x + cameraPos.x,
        parts_[i].collisionLocalPos.y + bossPos.y + cameraPos.y,
        parts_[i].collisionLocalPos.z + bossPos.z + cameraPos.z
    };

    volume.position = partWorldPos;
    volume.width = parts_[i].radiusX;
    volume.height = parts_[i].radiusY;
    // 奥行き（厚み）の設定。1.0f〜2.0f程度持たせると突き抜けにくくなります
    volume.depth = 2.0f;
    volume.partId = i;

   if (parts_[i].isWeakPoint) {
        volume.shape = CollisionShape::kBox;
        volume.normal = { 0.0f, 0.0f, -1.0f }; // 正面向き
    } else {
        volume.shape = CollisionShape::kBox;
        // 重要：矩形になっても「反射に使うための法線」として外向きベクトルを入れておく
        volume.normal = Normalize(parts_[i].collisionLocalPos);
    }
    return volume;
}

Vector3 banana::GetWorldPosition() const
{
    Vector3 cameraPos = camera_->GetTranslate();

    return {
        baseTransform_.translate.x + cameraPos.x,
        baseTransform_.translate.y + cameraPos.y,
        baseTransform_.translate.z + cameraPos.z
    };
}

float banana::GetRadius() const
{
    return 2.0f;
}

bool banana::GetIsDead() const
{
    return isDead_;
}

std::vector<banana::CollisionVolume> banana::GetCollisionVolumes()
{
    std::vector<banana::CollisionVolume> volumes;
    Vector3 bossPos = baseTransform_.translate;
    Vector3 cameraPos = camera_->GetTranslate();

    // --- 1巡目：まず「皮(Plane)」だけをリストに追加する ---
    for (uint32_t i = 0; i < parts_.size(); ++i) {
        if (parts_[i].isWeakPoint)
            continue; // 本体は後回し
        if (parts_[i].PartsHp <= 0)
            continue; // 壊れている皮は判定を作らない

        banana::CollisionVolume vol = CreateVolumeFromPart(i, bossPos, cameraPos); // 共通処理へ
        volumes.push_back(vol);
    }

    // --- 2巡目：最後に「本体(Box)」を追加する ---
    for (uint32_t i = 0; i < parts_.size(); ++i) {
        if (!parts_[i].isWeakPoint)
            continue;

        banana::CollisionVolume vol = CreateVolumeFromPart(i, bossPos, cameraPos);
        volumes.push_back(vol);
    }

    return volumes;
}

Vector3 banana::GetBodyWorldPosition() const
{
    Vector3 cameraPos = camera_->GetTranslate();

    for (const auto& part : parts_) {
        // 本体（弱点属性を持つパーツ）を探す
        if (part.isWeakPoint) {
            // Update関数と同じ計算式でワールド座標を算出
            Vector3 worldPos = {
                part.transform.translate.x + baseTransform_.translate.x + cameraPos.x,
                part.transform.translate.y + baseTransform_.translate.y + cameraPos.y,
                part.transform.translate.z + baseTransform_.translate.z + cameraPos.z
            };
            return worldPos;
        }
    }

    // 万が一見つからなかった場合はベース座標を返す
    return baseTransform_.translate;
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
        // ボス全体の共有体力を減らす
        health_ -= bullet->GetDamage();

        // 共有体力が0以下になったらボス撃破
        if (health_ <= 0) {
            health_ = 0;
            isDead_ = true;
        }

        return true; // プレイヤーの弾を消滅させる

    } else {
        hitPart.PartsHp -= bullet->GetDamage();
        if (hitPart.PartsHp >= 0) {
            BulletMirror(volume, bullet);
        } else if (hitPart.PartsHp <= 0 && hitPart.repairTimer <= 0.0f) {
            // --- 変更点9: 皮が破壊された瞬間に修理タイマーをセット ---
            hitPart.repairTimer = BossPart::kRepairTime;
        }

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
            newBulletEnemy->SetObject("bossBananAttack.obj");
            break;
        }

        // 弱点が攻撃をする
        newBulletEnemy->SetBulletAcceleration(Vector3(0.0f, 0.0f, -0.08f));
        newBulletEnemy->SetTargetPosition(player_->GetPosition());
        newBulletEnemy->SetAcceleration(0.5f);
        newBulletEnemy->SetUpgrade(0.0f);
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
    /*baseTransform_.translate += baseMove / 60.0f;
    if (baseTransform_.translate.x <= -5.0f || baseTransform_.translate.x >= 5.0f) {
        baseMove.x *= -1.0f;
    }
    if (baseTransform_.translate.y <= -5.0f || baseTransform_.translate.y >= 5.0f) {
        baseMove.y *= -1.0f;
    }*/
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
