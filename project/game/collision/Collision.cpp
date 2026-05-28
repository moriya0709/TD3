#include "Collision.h"
#include "../enemy/Boss/type/banana.h"
#include "../enemy/Boss/type/grapesBoss.h"
#include "../enemy/Enemy.h"
#include "../enemy/EnemyBullet.h"
#include "Player.h"
#include "PlayerBullet.h"
#include <cmath>

void CheckCollisionPlayerEnemy(Player* player, const std::list<std::shared_ptr<Enemy>>& enemies)
{
    Vector3 playerPos = player->GetPosition();
    float playerSize = player->GetHitSize();

    for (const auto& enemy : enemies) {

        // 死んでいる場合スキップを入れる

        Vector3 enemyPos = enemy->GetWorldPosition();
        float enemySize = enemy->GetRadius();

        Vector3 num = { playerPos.x - enemyPos.x, playerPos.y - enemyPos.y, playerPos.z - enemyPos.z };

        float distance = sqrtf(num.x * num.x + num.y * num.y + num.z * num.z);

        if (distance <= playerSize + enemySize) {
            // hit

            // 敵の攻撃力を受け取る

            int Damge = enemy->GetDameg();
            player->Damage(Damge);

            // ★ ダメージエフェクトを最大化（トリガー）
            PostEffect::GetInstance()->SetDamageEffectRatio(1.0f);
        }
    }
}

void CheckCollisionPlayerEnemyBullet(Player* player, const std::list<std::shared_ptr<Enemy>>& enemies)
{
    Vector3 playerPos = player->GetPosition();
    float playerSize = player->GetHitSize();

    for (const auto& enemyBullets_ : enemies) {

        // 死んでいる場合スキップを入れる

        const std::vector<std::unique_ptr<EnemyBullet>>& enemyBullet_ = enemyBullets_->GetBullets();
        for (const auto& bullet : enemyBullet_) {
            // 弾の座標
            Vector3 BulletPos = bullet->GetWorldPosition();
            float enemySize = bullet->GetRadius();

            Vector3 num = { playerPos.x - BulletPos.x, playerPos.y - BulletPos.y, playerPos.z - BulletPos.z };

            float distance = sqrtf(num.x * num.x + num.y * num.y + num.z * num.z);

            if (distance <= playerSize + enemySize) {
                // hit

                // 敵の攻撃力を受け取る
                int Damge = enemyBullets_->GetDameg();
                player->Damage(Damge);

                // 弾の消す処理
                bullet->OnCollision();

                // ダメージエフェクト(色収差、ビネット)
                if (distance <= playerSize + enemySize) {
                    // hit
                    player->Damage(Damge);
                    bullet->OnCollision();

                    // ★ ダメージエフェクトを最大化（トリガー）
                    PostEffect::GetInstance()->SetDamageEffectRatio(1.0f);
                }
            }
        }
    }
}

void CheckCollisionPlayerBulletEnemy(Player* player, const std::list<std::shared_ptr<Enemy>>& enemies, std::unique_ptr<ParticleEmitter>& hitEffect)
{
    for (const auto& bullet : player->GetBullets()) {
        // すでに当たって消える予定の弾はスキップ
        if (!bullet->IsActive())
            continue;

        Vector3 bulletPos = bullet->GetPosition();
        Vector3 preBulletPos = bullet->GetPreviousPosition();
        float bulletSize = bullet->GetHitSize();

        for (const auto& enemy : enemies) {
            // 敵が死んでいる場合はスキップ（敵側にAliveフラグ等がある前提）
            if (!enemy->GetIsAlive())
                continue;

            Vector3 enemyPos = enemy->GetWorldPosition();
            float enemySize = enemy->GetRadius();

            // 距離の計算
            Vector3 vecAB = { bulletPos.x - preBulletPos.x, bulletPos.y - preBulletPos.y, bulletPos.z - preBulletPos.z };
            float lenSqAB = vecAB.x * vecAB.x + vecAB.y * vecAB.y + vecAB.z * vecAB.z;

            Vector3 closestPoint;

            if (lenSqAB < 0.0001f) {
                closestPoint = preBulletPos;
            } else {
                // 2. 前フレーム位置から敵の中心へのベクトル (ベクトルAP)
                Vector3 vecAP = { enemyPos.x - preBulletPos.x, enemyPos.y - preBulletPos.y, enemyPos.z - preBulletPos.z };

                // 3. 内積（射影）の計算
                float dot = vecAP.x * vecAB.x + vecAP.y * vecAB.y + vecAP.z * vecAB.z;

                // 4. 線分上のどの位置（比率 t）に敵が一番近いかを計算
                float t = dot / lenSqAB;

                // 線分の外に出ないよう 0.0f 〜 1.0f にクランプする
                if (t < 0.0f)
                    t = 0.0f;
                if (t > 1.0f)
                    t = 1.0f;

                // 5. 比率 t を元に、線分上の「最も敵に近い最近傍点」を割り出す
                closestPoint.x = preBulletPos.x + t * vecAB.x;
                closestPoint.y = preBulletPos.y + t * vecAB.y;
                closestPoint.z = preBulletPos.z + t * vecAB.z;
            }

            // 6. 敵の中心と、弾の軌跡上の最近傍点との距離を計算
            Vector3 diff = { closestPoint.x - enemyPos.x, closestPoint.y - enemyPos.y, closestPoint.z - enemyPos.z };
            float distance = sqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);

            // 衝突判定
            if (distance <= bulletSize + enemySize) {
                // --- 修正ポイント ---
                bullet->SetActive(false); // 弾側のフラグをisActive = falseにするメソッド

                Vector3 bulletVelocity = bullet->GetVelocity();

                enemy->OnCollision(int(bullet->GetDamage()), closestPoint, bulletVelocity); // 敵側のダメージ処理を呼び出す（例としてプレイヤーの弾の数を渡す）

                // ヒットエフェクトの発生
                hitEffect->SetTranslate(closestPoint);
                hitEffect->Update(); // 発生

                break; // この弾は消えるので、他の敵との判定は不要
            }
        }
    }
}

void CheckCollisionPlayerBulletBossEnemy(Player* player, const std::list<std::shared_ptr<grapesBoss>>& enemies, std::unique_ptr<ParticleEmitter>& hitEffect)
{
    for (const auto& bullet : player->GetBullets()) {
        if (!bullet->IsActive())
            continue;

        Vector3 bulletPos = bullet->GetPosition();
        Vector3 preBulletPos = bullet->GetPreviousPosition();
        float bulletSize = bullet->GetHitSize();

        for (const auto& enemy : enemies) {
            // ボスから「今チェックすべき判定の球」を全部もらう
            auto volumes = enemy->GetCollisionVolumes();

            for (const auto& volume : volumes) {

                // 距離の計算
                Vector3 vecAB = { bulletPos.x - volume.position.x, bulletPos.y - volume.position.y, bulletPos.z - volume.position.z };
                float lenSqAB = vecAB.x * vecAB.x + vecAB.y * vecAB.y + vecAB.z * vecAB.z;

                Vector3 closestPoint;

                if (lenSqAB < 0.0001f) {
                    closestPoint = preBulletPos;
                } else {
                    // 2. 前フレーム位置から敵の中心へのベクトル (ベクトルAP)
                    Vector3 vecAP = { volume.position.x - preBulletPos.x, volume.position.y - preBulletPos.y, volume.position.z - preBulletPos.z };

                    // 3. 内積（射影）の計算
                    float dot = vecAP.x * vecAB.x + vecAP.y * vecAB.y + vecAP.z * vecAB.z;

                    // 4. 線分上のどの位置（比率 t）に敵が一番近いかを計算
                    float t = dot / lenSqAB;

                    // 線分の外に出ないよう 0.0f 〜 1.0f にクランプする
                    if (t < 0.0f)
                        t = 0.0f;
                    if (t > 1.0f)
                        t = 1.0f;

                    // 5. 比率 t を元に、線分上の「最も敵に近い最近傍点」を割り出す
                    closestPoint.x = preBulletPos.x + t * vecAB.x;
                    closestPoint.y = preBulletPos.y + t * vecAB.y;
                    closestPoint.z = preBulletPos.z + t * vecAB.z;
                }

                Vector3 diff = { closestPoint.x - volume.position.x, closestPoint.y - volume.position.y, closestPoint.z - volume.position.z };
                float distance = sqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);

                // 当たった場合
                if (distance <= bulletSize + volume.radius) {

                    // ボスに「このパーツに当たったぞ」と報告し、リアクションを任せる
                    if (enemy->OnCollision(volume, bullet.get())) {
                        bullet->SetActive(false); // 弾を消す

                        // ヒットエフェクトの発生
                        hitEffect->SetTranslate(closestPoint);
                        hitEffect->Update(); // 発生

                        break; // この弾の判定は終了
                    }
                }
            }
            if (!bullet->IsActive())
                break;
        }
    }
}

void CheckCollisionPlayerBossEnemy(Player* player, const std::list<std::shared_ptr<grapesBoss>>& enemies)
{
    Vector3 playerPos = player->GetPosition();

    for (const auto& boss : enemies) {
        if (boss->GetIsDead())
            continue;

        const std::vector<std::unique_ptr<EnemyBullet>>& Bullets_ = boss->GetBullets();

        for (const auto& Bullet : Bullets_) {
            if (!Bullet->GetIsActive())
                continue;

            Vector3 BossBullet = Bullet->GetWorldPosition();

            Vector3 diff = { playerPos - BossBullet };
            float distnance = sqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
            if (distnance <= player->GetHitSize() + Bullet->GetRadius()) {

                // 敵の攻撃力を受け取る
                int Damge = boss->GetDameg();
                player->Damage(Damge);

                // 弾の消す処理
                Bullet->OnCollision();

                // ★ ダメージエフェクトを最大化（トリガー）
                PostEffect::GetInstance()->SetDamageEffectRatio(1.0f);
            }
        }
    }
}

void CheckCollisionPlayerBossEnemyBullet(Player* player, const std::list<std::shared_ptr<grapesBoss>>& enemies)
{
    for (const auto& boss : enemies) {
        if (boss->GetIsDead())
            continue;

        auto volumes = boss->GetCollisionVolumes();

        for (const auto& volume : volumes) {

            Vector3 playerPos = player->GetPosition();

            Vector3 diff = { playerPos - volume.position };
            float distnance = sqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
            if (distnance <= player->GetHitSize() + boss->GetRadius()) {

                int Damge = boss->GetDamegBullet();
                player->Damage(Damge);

                // ★ ダメージエフェクトを最大化（トリガー）
                PostEffect::GetInstance()->SetDamageEffectRatio(1.0f);
            }
        }
    }
}

void CheckCollisionSpecialAtacgrapesEnemy(const std::list<std::shared_ptr<grapesBoss>>& enemies, std::unique_ptr<ParticleEmitter>& hitEffect)
{
    // 強制ダメージ
    for (const auto& enemy : enemies) {
        enemy->OnCollision(150);

        // ヒットエフェクトの発生（ジャストな衝突位置に表示される）
        hitEffect->SetTranslate(enemy->GetWorldPosition());
        hitEffect->Update();
    }
}

void CheckCollisionPlayerBulletBananaBoss(Player* player, const std::list<std::shared_ptr<banana>>& enemies, std::unique_ptr<ParticleEmitter>& hitEffect)
{
    const auto& playerBullets = player->GetBullets();

    for (auto& boss : enemies) {
        if (boss->GetIsDead())
            continue;
        std::vector<banana::CollisionVolume> volumes = boss->GetCollisionVolumes();

        for (auto& bullet : playerBullets) {
            if (!bullet->IsActive())
                continue;

            Vector3 bPos = bullet->GetPosition();
            Vector3 preBulletPos = bullet->GetPreviousPosition();
            float bRad = bullet->GetHitSize();

            for (const auto& volume : volumes) {

                // =================================================================
                // 【弾抜け対策】OBBのローカル空間へ両フレームの座標を変換して線分判定
                // =================================================================

                // 1. ボスの中心からの相対座標を計算（現フレーム ＆ 前フレーム）
                Vector3 relPos = { bPos.x - volume.position.x, bPos.y - volume.position.y, bPos.z - volume.position.z };
                Vector3 relPrePos = { preBulletPos.x - volume.position.x, preBulletPos.y - volume.position.y, preBulletPos.z - volume.position.z };

                // 2. ボスの各軸（axes）に投影してローカル座標を求める
                Vector3 localPos;
                localPos.x = relPos.x * volume.axes[0].x + relPos.y * volume.axes[0].y + relPos.z * volume.axes[0].z;
                localPos.y = relPos.x * volume.axes[1].x + relPos.y * volume.axes[1].y + relPos.z * volume.axes[1].z;
                localPos.z = relPos.x * volume.axes[2].x + relPos.y * volume.axes[2].y + relPos.z * volume.axes[2].z;

                Vector3 localPrePos;
                localPrePos.x = relPrePos.x * volume.axes[0].x + relPrePos.y * volume.axes[0].y + relPrePos.z * volume.axes[0].z;
                localPrePos.y = relPrePos.x * volume.axes[1].x + relPrePos.y * volume.axes[1].y + relPrePos.z * volume.axes[1].z;
                localPrePos.z = relPrePos.x * volume.axes[2].x + relPrePos.y * volume.axes[2].y + relPrePos.z * volume.axes[2].z;

                // 3. ローカル空間内での弾の移動ベクトルを算出
                Vector3 localVec = { localPos.x - localPrePos.x, localPos.y - localPrePos.y, localPos.z - localPrePos.z };
                float lenSq = localVec.x * localVec.x + localVec.y * localVec.y + localVec.z * localVec.z;

                Vector3 closestLocalPos; // 軌跡（線分）上でボスの中心に最も近い点

                if (lenSq < 0.0001f) {
                    closestLocalPos = localPos;
                } else {
                    // 前フレーム位置からボスの中心(0, 0, 0)へのベクトルに対する射影比率 t を計算
                    float dot = -(localPrePos.x * localVec.x + localPrePos.y * localVec.y + localPrePos.z * localVec.z);
                    float t = dot / lenSq;
                    t = std::clamp(t, 0.0f, 1.0f); // 線分の範囲内に制限

                    // 軌跡上の最近傍点を割り出す
                    closestLocalPos.x = localPrePos.x + t * localVec.x;
                    closestLocalPos.y = localPrePos.y + t * localVec.y;
                    closestLocalPos.z = localPrePos.z + t * localVec.z;
                }

                // 4. 割り出した最近傍点をローカルの箱のサイズ（AABB）に合わせてクランプ
                float closestX = std::clamp(closestLocalPos.x, -volume.width.x, volume.width.x);
                float closestY = std::clamp(closestLocalPos.y, -volume.width.y, volume.width.y);
                float closestZ = std::clamp(closestLocalPos.z, -volume.width.z, volume.width.z);

                // 5. 最短距離の判定
                Vector3 diff = { closestLocalPos.x - closestX, closestLocalPos.y - closestY, closestLocalPos.z - closestZ };
                float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

                if (distSq <= bRad * bRad) {
                    // ヒット！
                    if (boss->OnCollision(volume, bullet.get())) {
                        bullet->SetActive(false);

                        // ★向上ポイント：OBB表面の「実際に衝突したローカル座標」をワールド座標に逆変換
                        Vector3 worldHitPos;
                        worldHitPos.x = volume.position.x + closestX * volume.axes[0].x + closestY * volume.axes[1].x + closestZ * volume.axes[2].x;
                        worldHitPos.y = volume.position.y + closestX * volume.axes[0].y + closestY * volume.axes[1].y + closestZ * volume.axes[2].y;
                        worldHitPos.z = volume.position.z + closestX * volume.axes[0].z + closestY * volume.axes[1].z + closestZ * volume.axes[2].z;

                        // ヒットエフェクトの発生（ジャストな衝突位置に表示される）
                        hitEffect->SetTranslate(worldHitPos);
                        hitEffect->Update();
                    }
                    break; // この弾は消滅したため、他のボリュームとの判定をスキップ
                }
            }
        }
    }
}

void CheckCollisionPlayerBananaBoss(Player* player, const std::list<std::shared_ptr<banana>>& enemies)
{

    Vector3 pPos = player->GetPosition();
    float pRad = player->GetHitSize();

    for (auto& boss : enemies) {
        if (boss->GetIsDead())
            continue;

        std::vector<banana::CollisionVolume> volumes = boss->GetCollisionVolumes();

        for (const auto& volume : volumes) {
            // --- OBB判定ロジック ---

            // 1. OBBの中心から弾へのベクトル
            Vector3 relPos = { pPos.x - volume.position.x, pPos.y - volume.position.y, pPos.z - volume.position.z };

            // 2. 各軸に投影してローカル座標を求める（ドット積）
            Vector3 localPos;
            localPos.x = relPos.x * volume.axes[0].x + relPos.y * volume.axes[0].y + relPos.z * volume.axes[0].z;
            localPos.y = relPos.x * volume.axes[1].x + relPos.y * volume.axes[1].y + relPos.z * volume.axes[1].z;
            localPos.z = relPos.x * volume.axes[2].x + relPos.y * volume.axes[2].y + relPos.z * volume.axes[2].z;

            // 3. ローカル空間でのクランプ（AABBと同じ手法）
            float closestX = std::clamp(localPos.x, -volume.width.x, volume.width.x);
            float closestY = std::clamp(localPos.y, -volume.width.y, volume.width.y);
            float closestZ = std::clamp(localPos.z, -volume.width.z, volume.width.z);

            // 4. 距離の判定
            Vector3 diff = { localPos.x - closestX, localPos.y - closestY, localPos.z - closestZ };
            float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

            if (distSq <= pRad * pRad) {
                // 敵の攻撃力を受け取る
                int Damge = boss->GetDameg();
                player->Damage(Damge);

                // ★ ダメージエフェクトを最大化（トリガー）
                PostEffect::GetInstance()->SetDamageEffectRatio(1.0f);
            }
        }
    }
}

void CheckCollisionPlayerBananaBossBullet(Player* player, const std::list<std::shared_ptr<banana>>& enemies)
{
    Vector3 pPos = player->GetPosition();
    float pRad = player->GetHitSize();

    for (auto& boss : enemies) {
        if (boss->GetIsDead())
            continue;

        const auto& bossBullets = boss->GetBullets();

        for (auto& bullet : bossBullets) {
            if (!bullet->GetIsActive())
                continue;

            Vector3 bPos = bullet->GetWorldPosition();
            float bRad = bullet->GetRadius(); // 敵弾の半径を取得

            // 球 vs 球の判定（距離の2乗で比較）
            Vector3 diff = { pPos.x - bPos.x, pPos.y - bPos.y, pPos.z - bPos.z };
            float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
            float radSum = pRad + bRad;

            if (distSq <= radSum * radSum) {
                int Damge = boss->GetDamegBullet();

                player->Damage(Damge); // プレイヤーにダメージ

                bullet->OnCollision(); // 敵の弾を消す（関数名は適宜合わせてください）

                // ★ ダメージエフェクトを最大化（トリガー）
                PostEffect::GetInstance()->SetDamageEffectRatio(1.0f);
            }
        }
    }
}

void CheckCollisionSpecialAtackbananaEnemy(const std::list<std::shared_ptr<banana>>& enemies, std::unique_ptr<ParticleEmitter>& hitEffect)
{
    for (auto& boss : enemies) {
        if (boss->GetIsDead())
            continue;

        boss->OnCollision(150);
        std::vector<banana::CollisionVolume> volumes = boss->GetCollisionVolumes();

        for (const auto& volume : volumes) {

            hitEffect->SetTranslate(volume.position);
            hitEffect->Update();

            break; // この弾は消滅したため、他のボリュームとの判定をスキップ
        }
    }
}

void CheckCollisionPlayerBulletEnemyBullet(std::list<PlayerBullet*> playerBullet, std::vector<EnemyBullet*> enemyBullet) { }

void CheckCollisionSpecialAtackEnemy(const std::list<std::shared_ptr<Enemy>>& enemies)
{
    // 存在するすべての敵に当たる
    for (const auto& enemy : enemies) {
        // 敵が死んでいる場合はスキップ（敵側にAliveフラグ等がある前提）
        if (!enemy->GetIsAlive())
            continue;
        // 敵に爆弾のダメージを与える
        enemy->OnCollision(150, enemy->GetWorldPosition(), Vector3 { 0, 0, 0 }); // 例として100ダメージを与える
    }
}
