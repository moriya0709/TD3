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
    baseTransform_.translate = pos;

    BehaviorchangeTimer = kBehaviorchangeTimer;
    interval = maxInterval;

    isAvile = true;

    stemobject = std::make_unique<Object>();
    stemobject->Initialize(camera_);
    stemobject->SetModel("bossGrapesBranch.obj");
    stemobject->SetScale(baseTransform_.scale);
    stemobject->SetRotate(baseTransform_.rotate);

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
            p.object->Update();

            parts_.push_back(std::move(p)); // unique_ptrを含むので std::move
        }
    }
    currentWeakPointIndex_ = 1; // 初期状態の本体インデックス
}

void grapesBoss::Update()
{
    std::random_device seedGenerator;
    std::mt19937 randomEngine(seedGenerator());
    std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);

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

    const Matrix4x4& camMat = camera_->GetWorldMatrix();
    Vector3 cameraRot = camera_->GetRotate();

    Vector3 stemLocalPos = baseTransform_.translate + baseStem;
    stemobject->SetTranslate(TransformCoord(stemLocalPos, camMat));
    // 茎もカメラ目線にするならカメラの回転をセット
    stemobject->SetRotate(cameraRot);
    stemobject->Update();

    for (auto& part : parts_) {
        // 1. 位置の追従（ローカル座標をワールド座標に変換）
        Vector3 partLocalPos = baseTransform_.translate + part.transform.translate;
        Vector3 worldPos = TransformCoord(partLocalPos, camMat);
        if (behavior_ == Behavior::kDefeated) {
            worldPos.x += distribution(randomEngine);
            worldPos.y += distribution(randomEngine);
            worldPos.z += distribution(randomEngine);
        }
        part.object->SetTranslate(worldPos);

        float xFlip = 1.0f;
        // Y回転が pi (180度) に近いかどうかで判定
        if (std::abs(part.transform.rotate.y - (float)std::numbers::pi) < 0.1f) {
            xFlip = -1.0f; // ダミーの場合はXの傾きを逆にする
        }

        // 2. 回転の追従（カメラ目線 + パーツ固有のアニメーション回転）
        // ※エンジン仕様により、オイラー角の単純加算で破綻する場合はクォータニオンや行列合成が必要です
        Vector3 finalRot = {
            cameraRot.x * xFlip + part.transform.rotate.x,
            cameraRot.y + part.transform.rotate.y,
            cameraRot.z + part.transform.rotate.z
        };

        if (behavior_ == Behavior::kDefeated) {
            finalRot.x += distribution(randomEngine);
            finalRot.y += distribution(randomEngine);
            finalRot.z += distribution(randomEngine);
        }
        part.object->SetRotate(finalRot);

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

bool grapesBoss::OnCollision(const grapesBoss::CollisionVolume& volume, PlayerBullet* bullet)
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

        // 弾のダメージを取得
        int Damage = bullet->GetDamage();

        this->health_ -= Damage;

        if (this->health_ <= 0 || isDead_) {
            this->health_ = 0; // マイナスにならないよう補正
            behaviorRequest_ = Behavior::kDefeated;
            deadTimer_ = kdeadTimer_;
            isDead_ = true;
        }

        return true; // プレイヤーの弾を消滅させる

    } else {
        BulletMirror(volume, bullet);
        // ★ プレイヤーの弾自体はここで「消滅」させるため true を返す
        return true;
    }
}

std::vector<grapesBoss::CollisionVolume> grapesBoss::GetCollisionVolumes()
{
    std::vector<CollisionVolume> volumes;
    const Matrix4x4& camMat = camera_->GetWorldMatrix();

    // ボスの現在の中心座標 (GrapesBoss が持っている Transform)
    Vector3 bossPos = baseTransform_.translate;

    for (uint32_t i = 0; i < parts_.size(); ++i) {
        CollisionVolume volume;

        Vector3 partLocalPos = baseTransform_.translate + parts_[i].transform.translate;
        volume.position = TransformCoord(partLocalPos, camMat);

        volume.radius = parts_[i].radius;
        volume.partId = i;

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

            const Matrix4x4& camMat = camera_->GetWorldMatrix();
            Vector3 spawnWorldPos = TransformCoord(baseTransform_.translate + part.transform.translate, camMat);

            newBulletEnemy->Initialize(camera_, spawnWorldPos);
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
    const float deltaTime = 1.0f / 60.0f;

    // カメラの現在の回転を取得
    Vector3 cameraRot = camera_->GetRotate();

    for (auto& part : parts_) {
        // --- 1. 回転アニメーション（補間）の計算 ---
        if (part.isAnimating) {
            float currentY = part.transform.rotate.y;
            float targetY = part.targetRotate.y;
            float diffY = targetY - currentY;
            float step = rotationSpeed * deltaTime;

            if (std::abs(diffY) <= step) {
                part.transform.rotate.y = targetY;
                part.isAnimating = false;
            } else {
                part.transform.rotate.y += (diffY > 0.0f) ? step : -step;
            }
        }

        // --- 2. 【重要】カメラの回転をベースに、アニメーションの回転を足す ---
        // これにより、カメラがどこを向いていても、パーツはその方向を基準に回転します。
        Vector3 finalRotation = {
            cameraRot.x + part.transform.rotate.x,
            cameraRot.y + part.transform.rotate.y,
            cameraRot.z + part.transform.rotate.z
        };

        // オブジェクトに最終的な回転をセット
        part.object->SetRotate(finalRotation);
    }
}

void grapesBoss::MoveRush()
{
    // プレイヤーのワールド座標
    Vector3 targetWorldPos = player_->GetPosition();

    const Matrix4x4& camMat = camera_->GetWorldMatrix();
    Matrix4x4 invCamMat = Inverse(camMat); // ローカルに戻すための逆行列

    bool isLock = false;

    for (auto& patr : parts_) {
        if (!patr.isDashing && !patr.isReturning && !patr.isDashschedule)
            continue;

        // 1体ずつの発射待機タイマー
        if (patr.isDashschedule && !isLock) {
            dashTimer -= 1.0f / 60.0f;
            isLock = true;
            if (dashTimer <= 0.0f) {
                dashTimer = kdashTimer;
                patr.isDashschedule = false;
                patr.isDashing = true;

                // 【重要】突進開始時に、現在のローカル速度をワールド速度に変換しておく
                // （もし初期速度 {0,0,-0.5} を使うなら、それをワールド方向に変換）
                Vector3 localVel = patr.velocity;
                // 方向だけをカメラの向きに合わせて変換
                patr.velocity.x = localVel.x * camMat.m[0][0] + localVel.y * camMat.m[1][0] + localVel.z * camMat.m[2][0];
                patr.velocity.y = localVel.x * camMat.m[0][1] + localVel.y * camMat.m[1][1] + localVel.z * camMat.m[2][1];
                patr.velocity.z = localVel.x * camMat.m[0][2] + localVel.y * camMat.m[1][2] + localVel.z * camMat.m[2][2];
            }
        }

        if (patr.isDashing) {
            // --- A. 現在のワールド座標を算出 ---
            Vector3 currentLocalPos = baseTransform_.translate + patr.transform.translate;
            Vector3 currentWorldPos = TransformCoord(currentLocalPos, camMat);

            // --- B. ワールド空間での追跡計算 ---
            Vector3 toPlayerWorld = targetWorldPos - currentWorldPos;
            Vector3 directionWorld = Normalize(toPlayerWorld);

            // ホーミング性能の計算 (既存ロジック)
            float hPower = khomingPower;
            if (targetWorldPos.x >= 7.0f)
                hPower = 0.010f; // 適宜調整

            // ワールド速度を更新
            patr.velocity.x += directionWorld.x * hPower;
            patr.velocity.y += directionWorld.y * hPower;
            patr.velocity.z += directionWorld.z * hPower;

            float speed = sqrtf(Dot(patr.velocity, patr.velocity));
            if (speed > maxSpeed) {
                patr.velocity = Normalize(patr.velocity) * maxSpeed;
            }

            // ワールド座標を移動させる
            currentWorldPos.x += patr.velocity.x;
            currentWorldPos.y += patr.velocity.y;
            currentWorldPos.z += patr.velocity.z;

            // --- C. 移動後のワールド座標を「カメラのローカル座標」へ逆変換して保存 ---
            // これにより、Update関数の TransformCoord で正しく描画される
            Vector3 nextLocalPos = TransformCoord(currentWorldPos, invCamMat);
            patr.transform.translate = nextLocalPos - baseTransform_.translate;

            // --- D. カメラ通過判定 (ワールド空間で行う) ---
            Vector3 cameraPos = camera_->GetTranslate();
            Vector3 camForward = { camMat.m[2][0], camMat.m[2][1], camMat.m[2][2] };
            Vector3 toPart = currentWorldPos - cameraPos;

            if (Dot(camForward, toPart) < -5.0f) { // カメラの5ユニット後ろに行ったら
                patr.isDashing = false;
                patr.isReturning = true;
                patr.returnTimer = 0.0f;
                patr.startReturnPos = patr.transform.translate; // 通過した瞬間のローカル座標を保存
            }
        } else if (patr.isReturning) {
            // 帰還は「ボスの定位置（カメラ相対）」に戻る動作なので、
            // 既存のベジェ曲線（ローカル計算）のままでOKです。
            patr.returnTimer += 1.0f / 60.0f;
            if (patr.returnTimer >= 2.0f) {
                patr.transform.translate = patr.targetTranslate;
                patr.isReturning = false;
                patr.velocity = { 0, 0, 0 };
            } else {
                float t = patr.returnTimer / 2.0f;
                Vector3 p0 = patr.startReturnPos;
                Vector3 p2 = patr.targetTranslate;
                Vector3 p1 = { (p0.x + p2.x) * 0.5f, (p0.y + p2.y) * 0.5f - 50.0f, (p0.z + p2.z) * 0.5f };

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
    if (isDead_) {
        deadTimer_ -= 1.0f / 60.0f;
        if (deadTimer_ <= 0.0f) {
            isAlive_ = false;
        }
    }
}

bool grapesBoss::GetIsAlive() const
{
    return isAlive_;
}
