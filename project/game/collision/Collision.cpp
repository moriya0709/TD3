#include "Collision.h"
#include "../enemy/Enemy.h"
#include "../enemy/EnemyBullet.h"
#include "Player.h"
#include"PlayerBullet.h"
#include <cmath>

void CheckCollisionPlayerEnemy(Player* player, std::list<Enemy*> enemy) {}

void CheckCollisionPlayerEnemyBullet(Player* player, std::vector<EnemyBullet*> bullet) {}

void CheckCollisionPlayerBulletEnemy(std::list<PlayerBullet*> bullet, Enemy* enemy) {}

void CheckCollisionPlayerBulletEnemyBullet(std::list<PlayerBullet*> playerBullet, std::vector<EnemyBullet*> enemyBullet) {}
