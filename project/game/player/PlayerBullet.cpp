#include "PlayerBullet.h"

void PlayerBullet::Initialize(const Vector3& position, Camera* camera, const Vector2 reticlePosition, const float renge, const std::list<std::shared_ptr<Enemy>>& enemies) {};
void PlayerBullet::Update(Vector3 cmrvel) {};
void PlayerBullet::Draw3D() {};
void PlayerBullet::Draw2D() {}
void PlayerBullet::SetStatus(const float hommingAccuracy, const int damage) {};
bool PlayerBullet::IsActive() { return false; }
int PlayerBullet::GetPenetration() { return 0; };