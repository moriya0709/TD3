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
    float offsetRadius = 2.0f;

    for (int i = 0; i < pats; ++i) {
        BossPart p;
        p.transform.rotate = { 0.0f, 0.0f, 0.0f };
        p.transform.scale = { 14.0f, 14.0f, 14.0f };
        p.transform.translate = { 0.0f, 0.0f, 0.0f };

        // モデル生成
        p.object = std::make_unique<Object>();
        p.object->Initialize(camera_);
        if (i == 0)
            p.object->SetModel("bossBananPeelBuck.obj");
        else if (i == 1)
            p.object->SetModel("bossBananPeelLeft.obj");
        else if (i == 2)
            p.object->SetModel("bossBananPeelRight.obj");
        else if (i == 3)
            p.object->SetModel("bossBananBody.obj");

        p.object->SetScale(p.transform.scale);
        p.repairTimer = BossPart::kRepairTime;
        p.PartsHp = BossPart::kPartsHp;
        p.isWeakPoint = (i == 3); // i=3 を本体（弱点）とする

        // --- 逆三角形（Apex Forward）の配置ロジック ---
        if (i == 0) { // 正面頂点 (0度方向)
            p.baseAngle = 0.0f;
            p.collisionLocalPos = { (2.0f), 0.0f, offsetRadius };
        } else if (i == 1) { // 右後ろ (120度方向)
            p.baseAngle = 2.094f; // (2 * PI / 3)
            p.collisionLocalPos = { 2.0f + (offsetRadius * 0.866f), 0.0f, -offsetRadius * 0.5f };
        } else if (i == 2) { // 左後ろ (240度方向)
            p.baseAngle = 4.188f; // (4 * PI / 3)
            p.collisionLocalPos = { 2.0f + (-offsetRadius * 0.866f), 0.0f, -offsetRadius * 0.5f };
        } else { // 本体（中央）
            p.baseAngle = 0.0f;
            p.collisionLocalPos = { 2.0f, 0.0f, 0.0f };
        }

        parts_.push_back(std::move(p));
    }
}

void banana::Update()
{
    std::random_device seedGenerator;
    std::mt19937 randomEngine(seedGenerator());
    std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);

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
        float theta = part.collisionRotationY;

        // 2. 当たり判定の公転座標計算
        Vector3 rotatedPos;
        rotatedPos.x = part.collisionLocalPos.x * cosf(theta) + part.collisionLocalPos.z * sinf(theta);
        rotatedPos.y = part.collisionLocalPos.y;
        rotatedPos.z = -part.collisionLocalPos.x * sinf(theta) + part.collisionLocalPos.z * cosf(theta);

        // 3. 修理タイマー処理 (維持)
        if (!part.isWeakPoint && part.PartsHp <= 0) {
            part.repairTimer -= 1.0f / 60.0f;
            if (part.repairTimer <= 0.0f) {
                part.PartsHp = BossPart::kPartsHp;
                part.repairTimer = BossPart::kRepairTime;
            }
        }

        // 4. モデルの座標設定 (モデル自体は中心、または指定位置に固定)
        // ユーザー要望：モデルは回転させない = baseTransform_.translate の位置に固定
        Vector3 modelPos = baseTransform_.translate;
        if (behavior_ == Behavior::kDefeated) {
            modelPos.x += distribution(randomEngine);
            modelPos.y += distribution(randomEngine);
            modelPos.z += distribution(randomEngine);
        }
        part.object->SetTranslate(modelPos);
        // 回転も初期状態(0,0,0)を維持
        part.object->SetRotate({ 0.0f, 0.0f, 0.0f });
        part.object->Update();
    }
#ifdef USE_IMGUI
    // ImGuiでのステータス表示
    ImGui::Begin("Banana Boss Debug");
    ImGui::Text("Total Health: %d", health_);
    for (int i = 0; i < parts_.size(); i++) {
        ImGui::Separator();
        ImGui::Text("Part [%d] %s", i, parts_[i].isWeakPoint ? "BODY" : "PEEL");
        ImGui::Text("HP: %d", parts_[i].PartsHp);
        if (parts_[i].repairTimer > 0) {
            ImGui::Text("Repairing... %.1f", parts_[i].repairTimer);
        }
    }
    ImGui::End();
#endif
}

void banana::Draw3D()
{

    for (auto& part : parts_) {
        if (part.PartsHp > 1 || isAlive_ || !isDead_) {
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
    const auto& part = parts_[i];

    // 衝突判定専用の回転角を使用
    float currentRotation = part.collisionRotationY;
    float totalTheta = currentRotation + part.baseAngle;

    // 1. 中心座標の回転（公転）計算
    Vector3 rotatedLocalPos;
    rotatedLocalPos.x = part.collisionLocalPos.x * cosf(currentRotation) + part.collisionLocalPos.z * sinf(currentRotation);
    rotatedLocalPos.y = part.collisionLocalPos.y;
    rotatedLocalPos.z = -part.collisionLocalPos.x * sinf(currentRotation) + part.collisionLocalPos.z * cosf(currentRotation);

    volume.position = rotatedLocalPos + bossPos + cameraPos;

    // 2. OBBの3軸を決定 (totalThetaで自転させる)
    volume.axes[0] = { cosf(totalTheta), 0.0f, -sinf(totalTheta) }; // Right
    volume.axes[1] = { 0.0f, 1.0f, 0.0f }; // Up
    volume.axes[2] = { sinf(totalTheta), 0.0f, cosf(totalTheta) }; // Forward (法線)

    // 3. サイズ設定
    volume.width = part.isWeakPoint ? Vector3 { 0.5f, part.radiusY, 1.0f } : Vector3 { part.radiusX, part.radiusY, 1.0f };
    volume.shape = CollisionShape::kBox;
    volume.partId = i;
    volume.normal = volume.axes[2];

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

bool banana::GetIsAlive() const
{
    return isAlive_;
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
            behaviorRequest_ = Behavior::kDefeated;
            deadTimer_ = kdeadTimer_;
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

    if (isDead_) {
        deadTimer_ -= 1.0f / 60.0f;
        if (deadTimer_ <= 0.0f) {
            isAlive_ = false;
        }
    }
}
