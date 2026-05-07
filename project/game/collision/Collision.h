#include "CameraManager.h"
#include "ImGuiManager.h"
#include "Object.h"
#include "SoundManager.h"
#include "Sprite.h"


class Player;
class PlayerBullet;
class Enemy;
class EnemyBullet;
class grapesBoss;
class banana;

void CheckCollisionPlayerEnemy(Player* player, const std::list<std::shared_ptr<Enemy>>& enemies);
void CheckCollisionPlayerEnemyBullet(Player* player, const std::list<std::shared_ptr<Enemy>>& enemies);

void CheckCollisionPlayerBulletEnemy(Player* player, const std::list<std::shared_ptr<Enemy>>& enemies);
void CheckCollisionPlayerBulletEnemyBullet(std::list<PlayerBullet*> playerBullet, std::vector<EnemyBullet*> enemyBullet);
void CheckCollisionSpecialAtackEnemy(const std::list<std::shared_ptr<Enemy>>& enemies);

void CheckCollisionPlayerBulletBossEnemy(Player* player, const std::list<std::shared_ptr<grapesBoss>>& enemies);
void CheckCollisionPlayerBossEnemy(Player* player, const std::list<std::shared_ptr<grapesBoss>>& enemies);
void CheckCollisionPlayerBossEnemyBullet(Player* player, const std::list<std::shared_ptr<grapesBoss>>& enemies);


void CheckCollisionPlayerBulletBananaBoss(Player* player, const std::list<std::shared_ptr<banana>>& enemies);
void CheckCollisionPlayerBananaBoss(Player* player, const std::list<std::shared_ptr<banana>>& enemies);
void CheckCollisionPlayerBananaBossBullet(Player* player, const std::list<std::shared_ptr<banana>>& enemies);