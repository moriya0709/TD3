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

            int test = 1;
            player->Damage(test);
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
                int test = 1;
                player->Damage(test);

                // 弾の消す処理
                bullet->OnCollision();

                // ダメージエフェクト(色収差、ビネット)
                if (distance <= playerSize + enemySize) {
                    // hit
                    int test = 1;
                    player->Damage(test);
                    bullet->OnCollision();

                    // ★ ダメージエフェクトを最大化（トリガー）
                    PostEffect::GetInstance()->SetDamageEffectRatio(1.0f);
                }
            }
        }
    }
}

void CheckCollisionPlayerBulletEnemy(Player* player, const std::list<std::shared_ptr<Enemy>>& enemies)
{
    for (const auto& bullet : player->GetBullets()) {
        // すでに当たって消える予定の弾はスキップ
        if (!bullet->IsActive())
            continue;

        Vector3 bulletPos = bullet->GetPosition();
        float bulletSize = bullet->GetHitSize();

        for (const auto& enemy : enemies) {
            // 敵が死んでいる場合はスキップ（敵側にAliveフラグ等がある前提）
            if (!enemy->GetIsAlive())
                continue;

            Vector3 enemyPos = enemy->GetWorldPosition();
            float enemySize = enemy->GetRadius();

            // 距離の計算
            Vector3 diff = { bulletPos.x - enemyPos.x, bulletPos.y - enemyPos.y, bulletPos.z - enemyPos.z };
            float distance = sqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);

            // 衝突判定
            if (distance <= bulletSize + enemySize) {
                // --- 修正ポイント ---
                bullet->SetActive(false); // 弾側のフラグをisActive = falseにするメソッド

                Vector3 bulletVelocity = bullet->GetVelocity();

                enemy->OnCollision(int(bullet->GetDamage()), bulletPos, bulletVelocity); // 敵側のダメージ処理を呼び出す（例としてプレイヤーの弾の数を渡す）

                break; // この弾は消えるので、他の敵との判定は不要
            }
        }
    }
}

void CheckCollisionPlayerBulletBossEnemy(Player* player, const std::list<std::shared_ptr<grapesBoss>>& enemies)
{
    // 葡萄用の当たり判定なので触らないでください(継承クラスを破壊して個別にするため修正中です!!!!!!)
    for (const auto& bullet : player->GetBullets()) {
        if (!bullet->IsActive())
            continue;

        Vector3 bulletPos = bullet->GetPosition();
        float bulletSize = bullet->GetHitSize();

        for (const auto& enemy : enemies) {
            // ボスから「今チェックすべき判定の球」を全部もらう
            auto volumes = enemy->GetCollisionVolumes();

            for (const auto& volume : volumes) {

                Vector3 diff = { bulletPos.x - volume.position.x, bulletPos.y - volume.position.y, bulletPos.z - volume.position.z };
                float distance = sqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);

                // 当たった場合
                if (distance <= bulletSize + volume.radius) {

                    // ボスに「このパーツに当たったぞ」と報告し、リアクションを任せる
                    if (enemy->OnCollision(volume, bullet.get())) {
                        bullet->SetActive(false); // 弾を消す

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
                int test = 1;
                player->Damage(test);

                // 弾の消す処理
                Bullet->OnCollision();
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

                int test = 1;
                player->Damage(test);
            }
        }
    }
}

void CheckCollisionPlayerBulletBananaBoss(Player* player, const std::list<std::shared_ptr<banana>>& enemies)
{
    const auto& playerBullets = player->GetBullets();

    for (auto& boss : enemies) {
        if (boss->GetIsDead())
            continue;

        std::vector<banana::CollisionVolume> volumes = boss->GetCollisionVolumes();

        for (auto& bullet : playerBullets) {
            // 弾がすでに消滅している場合はスキップ（関数名はプロジェクトに合わせてください）
            if (!bullet->IsActive())
                continue;

            Vector3 bPos = bullet->GetPosition();
            float bRad = bullet->GetHitSize();

            for (const auto& volume : volumes) {
                bool isHit = false;

                if (volume.shape == banana::CollisionShape::kPlane) {
                    // ------------------------------------------
                    // 【面(皮) vs 球(弾) の判定】
                    // ------------------------------------------
                    Vector3 diff = { bPos.x - volume.position.x, bPos.y - volume.position.y, bPos.z - volume.position.z };

                    // 面からの距離 (内積)
                    float distToPlane = diff.x * volume.normal.x + diff.y * volume.normal.y + diff.z * volume.normal.z;

                    if (std::abs(distToPlane) <= bRad) {
                        // 面上のローカル座標へ投影
                        float localX = diff.x * volume.right.x + diff.y * volume.right.y + diff.z * volume.right.z;
                        float localY = diff.x * volume.up.x + diff.y * volume.up.y + diff.z * volume.up.z;

                        // 面の範囲内に収まっているか
                        if (std::abs(localX) <= volume.width && std::abs(localY) <= volume.height) {
                            isHit = true;
                        }
                    }
                } else if (volume.shape == banana::CollisionShape::kBox) {
                    // ------------------------------------------
                    // 【矩形/AABB(身) vs 球(弾) の判定】
                    // ------------------------------------------
                    // ※奥行き(Z)は横幅(width)と同じと仮定
                    float closestX = std::clamp(bPos.x, volume.position.x - volume.width, volume.position.x + volume.width);
                    float closestY = std::clamp(bPos.y, volume.position.y - volume.height, volume.position.y + volume.height);
                    float closestZ = std::clamp(bPos.z, volume.position.z - volume.width, volume.position.z + volume.width);

                    Vector3 diff = { bPos.x - closestX, bPos.y - closestY, bPos.z - closestZ };
                    float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

                    if (distSq <= bRad * bRad) {
                        isHit = true;
                    }
                }

                if (isHit) {
                    // ボス側の被弾処理を呼び出し、反射またはダメージ処理を行う
                    if (boss->OnCollision(volume, bullet.get())) {
                        bullet->SetActive(false); // 弾を消す
                    }
                    break; // 1つの弾は1フレームに1つの部位にのみ判定する
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
            bool isHit = false;

            if (volume.shape == banana::CollisionShape::kPlane) {
                // 皮とプレイヤーの当たり判定
                Vector3 diff = { pPos.x - volume.position.x, pPos.y - volume.position.y, pPos.z - volume.position.z };
                float distToPlane = diff.x * volume.normal.x + diff.y * volume.normal.y + diff.z * volume.normal.z;

                if (std::abs(distToPlane) <= pRad) {
                    float localX = diff.x * volume.right.x + diff.y * volume.right.y + diff.z * volume.right.z;
                    float localY = diff.x * volume.up.x + diff.y * volume.up.y + diff.z * volume.up.z;
                    if (std::abs(localX) <= volume.width && std::abs(localY) <= volume.height) {
                        isHit = true;
                    }
                }
            } else if (volume.shape == banana::CollisionShape::kBox) {
                // 身とプレイヤーの当たり判定
                float closestX = std::clamp(pPos.x, volume.position.x - volume.width, volume.position.x + volume.width);
                float closestY = std::clamp(pPos.y, volume.position.y - volume.height, volume.position.y + volume.height);
                float closestZ = std::clamp(pPos.z, volume.position.z - volume.width, volume.position.z + volume.width);

                Vector3 diff = { pPos.x - closestX, pPos.y - closestY, pPos.z - closestZ };
                if (diff.x * diff.x + diff.y * diff.y + diff.z * diff.z <= pRad * pRad) {
                    isHit = true;
                }
            }

            if (isHit) {
                // デバッグ出力
                char debugMsg[256];
                sprintf_s(debugMsg, "Hit! PartID: %u, Type: %s\n",
                    volume.partId,
                    (volume.shape == banana::CollisionShape::kPlane ? "Peel(Plane)" : "Body(Box)"));
                OutputDebugStringA(debugMsg); // Visual Studioの出力ウィンドウに表示

                int test = 1;

                player->Damage(test);
                break;
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
                int test = 1;

                player->Damage(test); // プレイヤーにダメージ

                bullet->OnCollision(); // 敵の弾を消す（関数名は適宜合わせてください）
            }
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

        enemy->OnCollision(10, enemy->GetWorldPosition(), Vector3 { 0, 0, 0 }); // 例として100ダメージを与える
    }
}
