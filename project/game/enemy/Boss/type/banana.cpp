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
        p.object->SetTranslate(p.transform.translate + baseTransform_.translate + cameraPos);
        p.object->Update();

        if (i == 0) {
            p.transformtaget = { 0.0f, 0.0f, 0.0f };
            p.transform.rotate = { 0.0f, (2.0f * (float)std::numbers::pi / (float)pats) * (float)i, 0.0f };
        } else if (i == 1) {
            p.transformtaget = { 4.0f, 0.0f, 0.0f };
            p.transform.rotate = { 0.0f, (2.0f * (float)std::numbers::pi / (float)pats) * (float)i, 0.0f };
        } else if (i == 2) {
            p.transformtaget = { 0.0f, 0.0f, 0.0f };
            p.transform.rotate = { 0.0f, (2.0f * (float)std::numbers::pi / (float)pats) * (float)i, 0.0f };
        } else if (i == 3) {
            // 本体
            p.transformtaget = { 0.0f, 0.0f, 0.0f };
            p.isWeakPoint = true;
        }

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

    for (auto& part : parts_) {
        Vector3 worldPos = part.transform.translate + baseTransform_.translate + cameraPos;
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

    // ==========================================
    // 1. 法線の取得（面の法線をそのまま使う）
    // ==========================================
    // volume.normal は前回の GetCollisionVolumes で計算した「面の向き」
    Vector3 normal = volume.normal;

    // ==========================================
    // 2. 反射ベクトルの計算
    // ==========================================
    // 公式: R = V - 2 * (V ・ N) * N
    float dot = bulletVelocity.x * normal.x + bulletVelocity.y * normal.y + bulletVelocity.z * normal.z;

    Vector3 reflectVelocity;
    reflectVelocity.x = bulletVelocity.x - 2.0f * dot * normal.x;
    reflectVelocity.y = bulletVelocity.y - 2.0f * dot * normal.y;
    reflectVelocity.z = bulletVelocity.z - 2.0f * dot * normal.z;

    // ゲーム的な調整（プレイヤーの方へ押し出す力）
    reflectVelocity.z *= 1.3f;

    // ==========================================
    // 3. 反射弾（敵の弾）の生成
    // ==========================================
    std::unique_ptr<HomingEnemyBullet> newBulletEnemy = std::make_unique<HomingEnemyBullet>();

    // 発生位置は「当たった位置」でOK
    newBulletEnemy->Initialize(camera_, bulletPos);

    // 【重要】加速度ではなく、初速として反射ベクトルを渡す
    // ※クラス設計によりますが、SetVelocityのようなものがあればそちらへ
    newBulletEnemy->SetBulletAcceleration(reflectVelocity);

    newBulletEnemy->SetTargetPosition(player_->GetPosition());
    newBulletEnemy->SetUpgrade(1.0f);
    newBulletEnemy->Update();

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

        // ボスのUpdateパーツ更新処理の中でのイメージ
        float angle = parts_[i].transform.rotate.y;

        CollisionVolume volume;
        // 面の向き（外側を向く法線ベクトル）
        volume.normal = { sinf(angle), 0.0f, cosf(angle) };
        // 面の横（法線と垂直な方向）
        volume.right = { cosf(angle), 0.0f, -sinf(angle) };
        // 面の縦（基本は上方向）
        volume.up = { 0.0f, 1.0f, 0.0f };

        

        // 2. 判定用のボリューム構造体を作成
        volume.position = partWorldPos; // 計算したワールド座標
        volume.width = parts_[i].radiusX; // パーツ個別の当たり判定サイズ
        volume.height = parts_[i].radiusY;
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

    if (!hitPart.isWeakPoint) {
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
        hitPart.PartsHp -= bullet->GetDamage();
        if (hitPart.PartsHp >= 0) {
            BulletMirror(volume, bullet);
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
