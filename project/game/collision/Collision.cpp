#include "Collision.h"
#include "../enemy/Enemy.h"
#include "../enemy/EnemyBullet.h"
#include "Player.h"
#include "PlayerBullet.h"
#include <cmath>

void CheckCollisionPlayerEnemy(Player* player, const std::list<std::unique_ptr<Enemy>>& enemies)
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

void CheckCollisionPlayerEnemyBullet(Player* player, const std::list<std::unique_ptr<Enemy>>& enemies)
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
            }
        }
    }
}


void CheckCollisionPlayerBulletEnemy(Player* player, const std::list<std::unique_ptr<Enemy>>& enemies) {

	for (const auto& bullet : player->GetBullets()) {
		// すでに当たって消える予定の弾はスキップ
		if (!bullet->IsActive())
			continue;

		Vector3 bulletPos = bullet->GetPosition();
		float bulletSize = bullet->GetHitSize();

		for (const auto& enemy : enemies) {
			// 敵が死んでいる場合はスキップ（敵側にAliveフラグ等がある前提）
			// if (!enemy->IsAlive()) continue;

			Vector3 enemyPos = enemy->GetWorldPosition();
			float enemySize = enemy->GetRadius();

			// 距離の計算
			Vector3 diff = {bulletPos.x - enemyPos.x, bulletPos.y - enemyPos.y, bulletPos.z - enemyPos.z};
			float distance = sqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);

			// 衝突判定
			if (distance <= bulletSize + enemySize) {
				// --- 修正ポイント ---
				bullet->SetActive(false); // 弾側のフラグをisActive = falseにするメソッド
				//enemy->OnCollision(player->GetBullets().size()); // 敵側のダメージ処理を呼び出す（例としてプレイヤーの弾の数を渡す）

				break; // この弾は消えるので、他の敵との判定は不要
			}
		}
	}
}
void CheckCollisionPlayerBulletEnemyBullet(std::list<PlayerBullet*> playerBullet, std::vector<EnemyBullet*> enemyBullet) {}

