#include "HomingEnemyBullet.h"
#include "../engine/math/Calc.h"

void HomingEnemyBullet::Initialize(Camera* camera, Vector3 Pos)
{
    camera_ = camera;

    transform_.scale = { 1.0f, 1.0f, 1.0f };
    transform_.rotate = { 0.0f, 0.0f, 0.0f };
    transform_.translate = Pos;

    object_ = std::make_unique<Object>();
    object_->Initialize(camera_);
    object_->SetModel("player.obj");
    object_->SetScale(transform_.scale);
    object_->SetRotate(transform_.rotate);
    object_->SetTranslate(transform_.translate);

    activeTimer = maxactiveTimer;
    acceleration_.z = 0.1f;
}

void HomingEnemyBullet::Update()
{
    activeTimer -= 1.0f / 60.0f;
    if (activeTimer <= 0.0f) {
        isAvile = false;
    }

    CheckCameraCulling();

    // ターゲットへのベクトル ＝ 目的地の座標 － 現在の座標
    Vector3 direction;
    direction.x = targetPos_.x - transform_.translate.x;
    direction.y = targetPos_.y - transform_.translate.y;
    direction.z = targetPos_.z - transform_.translate.z;

    // directionを「長さが1のベクトル（正規化ベクトル）」にする
    direction = Normalize(direction);

    // ホーミング性能
    float homingPower = 0.015f;

    // 加速度をターゲット方向に向ける
    acceleration_ = direction * homingPower;

    velocity_ += acceleration_;

    float currentSpeed = sqrtf(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
    // 弾の速さが最高速度を超えていたら、最高速度に制限する
    if (currentSpeed >= maxSpeed) {
        // 現在の進行方向（長さ1）を計算し、それに最高速度を掛ける
        Vector3 currentDir = Normalize(velocity_);
        velocity_ = currentDir * maxSpeed;
    }

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
    // 1. カメラの位置を取得
    Vector3 cameraPos = camera_->GetTranslate();

    // 2. カメラのワールド行列を取得
    const Matrix4x4& worldMat = camera_->GetWorldMatrix();

    // 3. ワールド行列から「正面（Z軸）の向き」を抜き出す
    Vector3 cameraForward = { worldMat.m[2][0], worldMat.m[2][1], worldMat.m[2][2] };

    // （念のため正規化して長さを1にしておく）
    cameraForward = Normalize(cameraForward);

    // 4. カメラから弾へ向かうベクトルを作る
    Vector3 toBullet;
    toBullet.x = transform_.translate.x - cameraPos.x;
    toBullet.y = transform_.translate.y - cameraPos.y;
    toBullet.z = transform_.translate.z - cameraPos.z;

    // 5. 内積（Dot）を計算して判定
    float dotProduct = cameraForward.x * toBullet.x + cameraForward.y * toBullet.y + cameraForward.z * toBullet.z;

    // 6. 内積がマイナス（カメラの後ろ）なら消滅させる
    // ※ 画面の端でフッと消えるのを防ぐため、少し余裕を持たせて -2.0f などにするのが実戦的です
    if (dotProduct < -2.0f) {
        isAvile = false;
    }
}

void HomingEnemyBullet::OnCollision()
{
    isAvile = false;
}
