#include "HomingEnemyBullet.h"
#include "../engine/math/Calc.h"

void HomingEnemyBullet::Initialize(Camera* camera, Vector3 Pos)
{
    camera_ = camera;

    transform_.scale = { 1.0f, 1.0f, 1.0f };
    transform_.rotate = { 0.0f, 0.0f, 0.0f };
    transform_.translate = Pos;

    NotLockTimer = 0.0f;

    object_ = std::make_unique<Object>();
    object_->Initialize(camera_);
    object_->SetModel("pepaBullet.obj");
    object_->SetScale(transform_.scale);
    object_->SetRotate(transform_.rotate);
    object_->SetTranslate(transform_.translate);

    activeTimer = maxactiveTimer;

    velocity_ = { 0.0f, 0.0f, 0.0f };
    acceleration_ = { 0.0f, 0.0f, 0.0f };
}

void HomingEnemyBullet::Update()
{
    activeTimer -= 1.0f / 60.0f;
    if (activeTimer <= 0.0f) {
        isAvile = false;
    }

    CheckCameraCulling();

    // ★タイマーによる条件分岐
    if (NotLockTimer <= 0.0f) {
        // 【タイマー終了後】プレイヤーへの誘導ベクトルを計算
        Vector3 direction;
        direction.x = targetPos_.x - transform_.translate.x;
        direction.y = targetPos_.y - transform_.translate.y;
        direction.z = targetPos_.z - transform_.translate.z;

        // 正規化
        direction = Normalize(direction);

        // 加速度をターゲット方向に向ける（追尾開始）
        acceleration_ = direction * homingPower * accelerationScalar;
    } else {
        // 【タイマー作動中】カウントダウンのみ行い、誘導加速度は 0 のまま（初速で直進）
        NotLockTimer -= 1.0f / 60.0f;
        acceleration_ = { 0.0f, 0.0f, 0.0f };
    }

    velocity_ += acceleration_;

    float currentSpeed = sqrtf(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
    // 弾の速さが最高速度を超えていたら、最高速度に制限する
    float totalMaxSpeed = maxSpeed + maxSpeedUpgrade; // 合計の限界値を出す
    if (currentSpeed >= totalMaxSpeed) {

        // 現在の進行方向（長さ1）を計算し、それに最高速度を掛ける
        Vector3 currentDir = Normalize(velocity_);
        velocity_ = currentDir * totalMaxSpeed;
    }

    Vector3 rotate;
    rotate.y = atan2(velocity_.x, velocity_.z);
    // 横軸方向の長さを求める
    float hypotXZ = std::hypot(velocity_.x, velocity_.z);
    rotate.x = atan2(-velocity_.y, hypotXZ);
    rotate.z = 0.0f;
    object_->SetRotate(rotate);

    transform_.translate += velocity_;
    object_->SetTranslate(transform_.translate);

    // 更新
    object_->Update();
}

void HomingEnemyBullet::Draw3D()
{
    // 3Dオブジェクト描画
    object_->Draw();
}

void HomingEnemyBullet::CheckCameraCulling()
{

    Vector3 cameraPos = camera_->GetTranslate();

    const Matrix4x4& worldMat = camera_->GetWorldMatrix();

    Vector3 cameraForward = { worldMat.m[2][0], worldMat.m[2][1], worldMat.m[2][2] };

    // 正規化
    cameraForward = Normalize(cameraForward);

    // べ黒る
    Vector3 toBullet;
    toBullet.x = transform_.translate.x - cameraPos.x;
    toBullet.y = transform_.translate.y - cameraPos.y;
    toBullet.z = transform_.translate.z - cameraPos.z;

    // 内積
    float dotProduct = cameraForward.x * toBullet.x + cameraForward.y * toBullet.y + cameraForward.z * toBullet.z;

    // カメラから離れているなら
    // カメラ+オフセット(5.0f)の分を含めた距離
    if (dotProduct < 4.5f) {
        isAvile = false;
    }
}

void HomingEnemyBullet::OnCollision()
{
    isAvile = false;
}
