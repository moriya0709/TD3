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

        // éÄÇÒÇ≈Ç¢ÇÈèÍçáÉXÉLÉbÉvÇì¸ÇÍÇÈ

        Vector3 enemyPos = enemy->GetWorldPosition();
        float enemySize = enemy->GetRadius();

        Vector3 num = { playerPos.x - enemyPos.x, playerPos.y - enemyPos.y, playerPos.z - enemyPos.z };

        float distance = sqrtf(num.x * num.x + num.y * num.y + num.z * num.z);

        if (distance <= playerSize + enemySize) {
            // hit

            // ìGÇÃçUåÇóÕÇéÛÇØéÊÇÈ

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

        // éÄÇÒÇ≈Ç¢ÇÈèÍçáÉXÉLÉbÉvÇì¸ÇÍÇÈ

        const std::vector<std::unique_ptr<EnemyBullet>>& enemyBullet_ = enemyBullets_->GetBullets();
        for (const auto& bullet : enemyBullet_) {
            // íeÇÃç¿ïW
            Vector3 BulletPos = bullet->GetWorldPosition();
            float enemySize = bullet->GetRadius();

            Vector3 num = { playerPos.x - BulletPos.x, playerPos.y - BulletPos.y, playerPos.z - BulletPos.z };

            float distance = sqrtf(num.x * num.x + num.y * num.y + num.z * num.z);

            if (distance <= playerSize + enemySize) {
                // hit

                // ìGÇÃçUåÇóÕÇéÛÇØéÊÇÈ
                int test = 1;
                player->Damage(test);

                // íeÇÃè¡Ç∑èàóù
                bullet->OnCollision();
            }
        }
    }
}

void CheckCollisionPlayerBulletEnemy(Player* player, const std::list<std::unique_ptr<Enemy>>& enemies)
{
    const std::list<std::unique_ptr<PlayerBullet>>& playerBullets = player->GetBullets();
    for (const auto& bullet : playerBullets) {
        Vector3 bulletPos = bullet->GetPosition();
        float bulletSize = bullet->GetHitSize();
        for (const auto& enemy : enemies) {
            Vector3 enemyPos = enemy->GetWorldPosition();
            float enemySize = enemy->GetRadius();
            Vector3 num = { bulletPos.x - enemyPos.x, bulletPos.y - enemyPos.y, bulletPos.z - enemyPos.z };
            float distance = sqrtf(num.x * num.x + num.y * num.y + num.z * num.z);
            if (distance <= bulletSize + enemySize) {
                int test = 1;
            }
        }
    }
}

void CheckCollisionPlayerBulletEnemyBullet(std::list<PlayerBullet*> playerBullet, std::vector<EnemyBullet*> enemyBullet) { }
