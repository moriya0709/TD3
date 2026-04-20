#include "Collision.h"
#include "../enemy/Boss/BossEnemy.h"
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

void CheckCollisionPlayerBulletBossEnemy(Player* player, const std::list<std::shared_ptr<BossEnemy>>& enemies)
{
    for (const auto& bullet : player->GetBullets()) {
        if (!bullet->IsActive())
            continue;

        Vector3 bulletPos = bullet->GetPosition();
        float bulletSize = bullet->GetHitSize();

        for (const auto& enemy : enemies) {
            auto volumes = enemy->GetCollisionVolumes();

            for (const auto& volume : volumes) {
                // 1. 面の中心から弾へのベクトル
                Vector3 v = { bulletPos.x - volume.position.x, bulletPos.y - volume.position.y, bulletPos.z - volume.position.z };

                // 2. 面の法線方向への距離（厚み方向の判定）
                // 内積を使って「面からどれだけ浮いているか」を出す
                float distNormal = v.x * volume.normal.x + v.y * volume.normal.y + v.z * volume.normal.z;

                // 弾の半径分、面から離れすぎていたら当たっていない
                if (fabsf(distNormal) > bulletSize)
                    continue;

                // 3. 面の横方向・縦方向の範囲内かチェック（投影）
                float distRight = v.x * volume.right.x + v.y * volume.right.y + v.z * volume.right.z;
                float distUp = v.x * volume.up.x + v.y * volume.up.y + v.z * volume.up.z;

                // 矩形の範囲（幅・高さ）に弾のサイズを考慮して判定
                if (fabsf(distRight) <= volume.width + bulletSize && fabsf(distUp) <= volume.height + bulletSize) {

                    if (enemy->OnCollision(volume, bullet.get())) {
                        bullet->SetActive(false);
                        break;
                    }
                }
            }
            if (!bullet->IsActive())
                break;
        }
    }
}

void CheckCollisionPlayerBossEnemy(Player* player, const std::list<std::shared_ptr<BossEnemy>>& enemies)
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

void CheckCollisionPlayerBossEnemyBullet(Player* player, const std::list<std::shared_ptr<BossEnemy>>& enemies)
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
