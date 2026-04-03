#include "CameraManager.h"
#include "ImGuiManager.h"
#include "Object.h"
#include "SoundManager.h"
#include "Sprite.h"


class Player;
class PlayerBullet;
class Enemy;
class EnemyBullet;
class BossEnemy;

void CheckCollisionPlayerEnemy(Player* player, const std::list<std::shared_ptr<Enemy>>& enemies);
void CheckCollisionPlayerEnemyBullet(Player* player, const std::list<std::shared_ptr<Enemy>>& enemies);

void CheckCollisionPlayerBulletEnemy(Player* player, const std::list<std::shared_ptr<Enemy>>& enemies);
void CheckCollisionPlayerBulletEnemyBullet(std::list<PlayerBullet*> playerBullet, std::vector<EnemyBullet*> enemyBullet);
void CheckCollisionSpecialAtackEnemy(const std::list<std::shared_ptr<Enemy>>& enemies);

void CheckCollisionPlayerBulletBossEnemy(Player* player, const std::list<std::shared_ptr<BossEnemy>>& enemies);
void CheckCollisionPlayerBossEnemy(Player* player, const std::list<std::shared_ptr<BossEnemy>>& enemies);
void CheckCollisionPlayerBossEnemyBullet(Player* player, const std::list<std::shared_ptr<BossEnemy>>& enemies);