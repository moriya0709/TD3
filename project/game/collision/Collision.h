#include "CameraManager.h"
#include "ImGuiManager.h"
#include "Object.h"
#include "SoundManager.h"
#include "Sprite.h"


class Player;
class PlayerBullet;
class Enemy;
class EnemyBullet;

void CheckCollisionPlayerEnemy(Player* player, std::list<Enemy*> enemy);
void CheckCollisionPlayerEnemyBullet(Player* player, std::vector<EnemyBullet*> bullet);

void CheckCollisionPlayerBulletEnemy(std::list<PlayerBullet*> bullet, Enemy* enemy);
void CheckCollisionPlayerBulletEnemyBullet(std::list<PlayerBullet*> playerBullet, std::vector<EnemyBullet*> enemyBullet);
