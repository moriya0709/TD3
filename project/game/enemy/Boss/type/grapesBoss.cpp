#define NOMINMAX

#include "grapesBoss.h"
#include "../../Normal/Bullet/HomingEnemyBullet.h"
#include "../../Normal/Bullet/TargetEnemyBullet.h"
#include "Player.h"

void grapesBoss::Initialize(Camera* camera, Vector3 pos, int health)
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

    float spacingX = 5.0f; // パーツ間の横の間隔
    float spacingY = 5.0f; // パーツ間の縦の間隔

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

            if (p.isWeakPoint) {
                p.transform.rotate = { 0.0f, 0.0f, 0.0f };
                p.targetRotate = { 0.0f, 0.0f, 0.0f };
            } else {
                p.transform.rotate = { 0.0f, (float)std::numbers::pi, 0.0f };
                p.targetRotate = { 0.0f, (float)std::numbers::pi, 0.0f };
            }

            p.isAnimating = false;

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
    currentWeakPointIndex_ = 1; // 初期状態の本体インデックス
}

void grapesBoss::Update()
{

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

    stemobject->SetTranslate(baseTransform_.translate + cameraPos + baseStem);
    stemobject->Update();
    for (auto& part : parts_) {
        Vector3 worldPos = part.transform.translate + baseTransform_.translate + cameraPos;
        part.object->SetTranslate(worldPos);
        part.object->Update();
    }
}

void grapesBoss::Draw3D()
{
    stemobject->Draw();

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
    newBulletEnemy->Initialize(camera_, bulletPos);
    newBulletEnemy->SetBulletAcceleration(reflectVelocity);
    newBulletEnemy->SetTargetPosition(player_->GetPosition());
    newBulletEnemy->SetUpgrade(1.0f);
    newBulletEnemy->Update();

    // GrapesBossクラスが持つ enemyBullet_ リストに追加
    enemyBullet_.push_back(std::move(newBulletEnemy));
}

void grapesBoss::WeakPointChange()
{
    int currentWeakIdx = -1;
    std::vector<int> dummyIndices;

    for (int i = 0; i < (int)parts_.size(); ++i) {
        if (parts_[i].isWeakPoint) {
            currentWeakIdx = i;
        } else {
            dummyIndices.push_back(i);
        }
    }

    if (currentWeakIdx == -1 || dummyIndices.empty())
        return;

    // 2. ダミーの中からランダムに新しい本体を1つ選ぶ
    std::random_device seed_gen;
    std::mt19937 engine(seed_gen());
    std::uniform_int_distribution<size_t> dist(0, dummyIndices.size() - 1);
    int newWeakIdx = dummyIndices[dist(engine)];

    // 3. 役割（フラグ）の入れ替え
    parts_[currentWeakIdx].isWeakPoint = false;
    parts_[newWeakIdx].isWeakPoint = true;

    // 4. --- アニメーションの目標角度を設定 ---

    // 元本体（今後はダミー）を Y軸PI度回転 に向かわせる
    parts_[currentWeakIdx].targetRotate.y = (float)std::numbers::pi;
    parts_[currentWeakIdx].isAnimating = true;

    // 新本体（今後は弱点）を Y軸0度回転 に向かわせる
    parts_[newWeakIdx].targetRotate.y = 0.0f;
    parts_[newWeakIdx].isAnimating = true;
}

Vector3 grapesBoss::GetWorldPosition() const
{
    return baseTransform_.translate;
}

float grapesBoss::GetRadius() const
{
    return 2.0f;
}

bool grapesBoss::GetIsDead() const
{
    return isDead_;
}

bool grapesBoss::OnCollision(const CollisionVolume& volume, PlayerBullet* bullet)
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
        Vector3 cameraPos = camera_->GetTranslate();

        Vector3 partWorldPos = {
            parts_[i].transform.translate.x + bossPos.x + cameraPos.x,
            parts_[i].transform.translate.y + bossPos.y + cameraPos.y,
            parts_[i].transform.translate.z + bossPos.z + cameraPos.z
        };

        // 2. 判定用のボリューム構造体を作成
        CollisionVolume volume;
        volume.position = partWorldPos; // 計算したワールド座標
        volume.width = parts_[i].radius;
        volume.height = parts_[i].radius; // パーツ個別の当たり判定サイズ
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

void grapesBoss::StartRush()
{
    // 1. 生きているパーツのインデックスをリストアップ
    std::vector<int> aliveIndices;
    for (int i = 0; i < (int)parts_.size(); ++i) {
        // すでに突進中なものは除外（連続で選ばれないように）
        if (!parts_[i].isDashing && !parts_[i].isReturning) {
            aliveIndices.push_back(i);
        }
    }

    // 2. シャッフルして先頭3つを選ぶ
    std::random_device seed_gen;
    std::mt19937 engine(seed_gen());
    std::shuffle(aliveIndices.begin(), aliveIndices.end(), engine);

    int count = std::min((int)aliveIndices.size(), 3); // 最大3体
    for (int i = 0; i < count; ++i) {
        int targetIdx = aliveIndices[i];
        parts_[targetIdx].isDashschedule = true;
        parts_[targetIdx].targetTranslate = parts_[targetIdx].transform.translate; // 座標を保存
        parts_[targetIdx].velocity = { 0, 0, -0.5f }; // 初速（プレイヤー方向へ）
    }
}

void grapesBoss::MoveUpdate()
{

    baseTransform_.translate += baseMove / 60.0f;
    if (baseTransform_.translate.x <= -5.0f || baseTransform_.translate.x >= 5.0f) {
        baseMove.x *= -1.0f;
    }
    if (baseTransform_.translate.y <= -5.0f || baseTransform_.translate.y >= 5.0f) {
        baseMove.y *= -1.0f;
    }

    RoatetUpdate();
}

void grapesBoss::RoatetUpdate()
{

    const float rotationSpeed = (float)std::numbers::pi * 2.0f;
    const float deltaTime = 1.0f / 60.0f; // フレーム間隔（仮）

    for (auto& part : parts_) {

        // --- 回転アニメーション処理 ---
        if (part.isAnimating) {
            float currentY = part.transform.rotate.y;
            float targetY = part.targetRotate.y;
            float diffY = targetY - currentY;

            // このフレームでの回転量
            float step = rotationSpeed * deltaTime;

            if (std::abs(diffY) <= step) {
                // 目標角度にほぼ着いたので、完全に一致させてアニメーションを終了
                part.transform.rotate.y = targetY;
                part.isAnimating = false;

            } else {
                // ターゲットに向かって少し回す
                if (diffY > 0.0f) {
                    part.transform.rotate.y += step;
                } else {
                    part.transform.rotate.y -= step;
                }
            }
        }

        part.object->SetRotate(part.transform.rotate);
    }
}

void grapesBoss::MoveRush()
{

    // 本体を含むランダムに3個突進をする敵を設定　→　2秒おきに順番に突進　→　最後に戻る際プレイヤー当たらないように下側(又は上側)に弧を描いて帰宅。

    Vector3 targetPos_;
    targetPos_ = player_->GetPosition();
    Vector3 cameraPos = camera_->GetTranslate();
    const Matrix4x4& worldMat = camera_->GetWorldMatrix();
    Vector3 cameraForward = { worldMat.m[2][0], worldMat.m[2][1], worldMat.m[2][2] };
    cameraForward = Normalize(cameraForward);
    bool isLock = false; // 1体ずつの更新にするため

    // ランダム関数とかでいいから3体突進する対象を決める

    // X = 7 Y = 3

    for (auto& patr : parts_) {

        // 突進も帰還もしていないなら何もしない
        if (!patr.isDashing && !patr.isReturning && !patr.isDashschedule)
            continue;

        if (patr.isDashschedule && !isLock) {
            dashTimer -= 1.0f / 60.0f;
            isLock = true;

            if (dashTimer <= 0.0f) {
                dashTimer = kdashTimer;
                patr.isDashschedule = false;
                patr.isDashing = true;
            }
        }

        if (patr.isDashing) {
            // ターゲットへのベクトル ＝ 目的地の座標 － 現在の座標
            Vector3 currentWorldPos = baseTransform_.translate + patr.transform.translate + cameraPos;
            Vector3 toPlayer = targetPos_ - currentWorldPos;
            Vector3 direction = Normalize(toPlayer);

            // ホーミングの変更
            if ((targetPos_.x + patr.targetTranslate.x) * 0.5f >= 7.0f) {
                homingPower = 0.010f;
                if ((targetPos_.y + patr.targetTranslate.y) * 0.5f >= 3.0f) {
                    homingPower = 0.015f;
                }

            } else {
                homingPower = khomingPower;
            }

            // 加速度をターゲット方向に向ける
            patr.velocity.x += direction.x * homingPower;
            patr.velocity.y += direction.y * homingPower;
            patr.velocity.z += direction.z * homingPower;

            // 速度超過だった場合の処理
            float currentSpeed = sqrtf(patr.velocity.x * patr.velocity.x + patr.velocity.y * patr.velocity.y + patr.velocity.z * patr.velocity.z);

            if (currentSpeed > maxSpeed) {
                patr.velocity = Normalize(patr.velocity) * maxSpeed;
            }

            patr.transform.translate += patr.velocity;

            // カメラの奥に行ったかどうか
            Vector3 toPart = (baseTransform_.translate + patr.transform.translate + cameraPos) - cameraPos;
            float dot = Dot(cameraForward, toPart);
            if (dot < -2.0f) {
                patr.isDashing = false;
                patr.isReturning = true;
                patr.returnTimer = 0.0f;
                patr.startReturnPos = patr.transform.translate; // 通過した瞬間のローカル座標を保存
            }
        } else if (patr.isReturning) {
            patr.returnTimer += 1.0f / 60.0f;

            if (patr.returnTimer >= 2.0f) {
                patr.transform.translate = patr.targetTranslate; // 本来の定位置
                patr.isReturning = false;
                patr.velocity = { 0, 0, 0 };
            } else {
                // P0: 画面外（通過地点）
                // P1: 制御点（大きく横に膨らませる）
                // P2: ボス本体の定位置
                Vector3 p0 = patr.startReturnPos;
                Vector3 p2 = patr.targetTranslate;
                Vector3 p1 = {
                    (p0.x + p2.x) * 0.5f, // Xは中間地点のまま（横には膨らませない）
                    (p0.y + p2.y) * 0.5f - 50.0f, // Yをマイナス方向に大きくズラす（下に膨らむ）
                    p0.z * 0.5f
                };

                // 2次ベジエ曲線公式: (1-t)^2*P0 + 2(1-t)t*P1 + t^2*P2
                float t = patr.returnTimer / 2.0f;
                patr.transform.translate.x = (1 - t) * (1 - t) * p0.x + 2 * (1 - t) * t * p1.x + t * t * p2.x;
                patr.transform.translate.y = (1 - t) * (1 - t) * p0.y + 2 * (1 - t) * t * p1.y + t * t * p2.y;
                patr.transform.translate.z = (1 - t) * (1 - t) * p0.z + 2 * (1 - t) * t * p1.z + t * t * p2.z;
            }
        }
    }
}

void grapesBoss::BehaviorStillness()
{
    // 弱点入れ替え
    WeakPointchangeTimer -= 1.0f / 60.0f;
    if (WeakPointchangeTimer <= 0.0f && intervalCount == 0) {
        WeakPointChange();
        WeakPointchangeTimer = ktWeakPointchangeTimer;
    }

    // 移動
    MoveUpdate();

    BehaviorchangeTimer -= 1.0f / 60.0f;
    if (BehaviorchangeTimer <= 0.0f && intervalCount == 0) {
        behaviorRequest_ = Behavior::kAttack;
        StartRush();
        BehaviorchangeTimer = kBehaviorchangeTimer;
    }
}

void grapesBoss::BehaviorAttack()
{
    // 突進が終わったらStillnessに切り替えにする
    std::vector<int> aliveIndices;
    for (int i = 0; i < (int)parts_.size(); ++i) {
        // すべてfasleならカウントを入れる
        if (!parts_[i].isDashing && !parts_[i].isReturning && !parts_[i].isDashschedule) {
            aliveIndices.push_back(i);
        }
    }
    if (aliveIndices.size() == (int)parts_.size()) {
        behaviorRequest_ = Behavior::kStillness;
    }

    // 回転のみ
    RoatetUpdate();

    MoveRush();
}

void grapesBoss::BehaviorShield()
{
}

void grapesBoss::BehaviorDefeated()
{
}
