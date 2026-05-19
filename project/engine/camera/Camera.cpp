#include "Camera.h"
#include "WindowAPI.h"

Camera* Camera::instance = nullptr;

Camera::Camera()
	: transform({{1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f}})
	, fovY_(0.45f)
	, aspectRatio_(float(WindowAPI::kClientWidth) / float (WindowAPI::kClientHeight))
	, nearClip_(0.1f)
	, farClip_(100.0f)
	, worldMatrix(MakeAffineMatrix(transform.scale, transform.rotate, transform.translate))
	, viewMatrix(Inverse(worldMatrix))
	, projectionMatrix(MakePerspectiveFovMatrix(fovY_, aspectRatio_, nearClip_, farClip_))
	, viewProjectionMatrix(Multiply(viewMatrix, projectionMatrix))
{}

void Camera::Update() {
	// ビュー行列
	worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	viewMatrix = Inverse(worldMatrix);

	// プロジェクション行列
	projectionMatrix = MakePerspectiveFovMatrix(fovY_,aspectRatio_,nearClip_,farClip_);

	// 合成行列
	viewProjectionMatrix = Multiply(viewMatrix,projectionMatrix);

}

const Vector2 Camera::WorldToScreen(const Vector3& worldPos) const { 
	Matrix4x4 vpMat = viewProjectionMatrix;
	Vector4 clipPos = {
		worldPos.x * vpMat.m[0][0] + worldPos.y * vpMat.m[1][0] + worldPos.z * vpMat.m[2][0] + vpMat.m[3][0],
		worldPos.x * vpMat.m[0][1] + worldPos.y * vpMat.m[1][1] + worldPos.z * vpMat.m[2][1] + vpMat.m[3][1],
		worldPos.x * vpMat.m[0][2] + worldPos.y * vpMat.m[1][2] + worldPos.z * vpMat.m[2][2] + vpMat.m[3][2],
		worldPos.x * vpMat.m[0][3] + worldPos.y * vpMat.m[1][3] + worldPos.z * vpMat.m[2][3] + vpMat.m[3][3]
	};
	if (clipPos.w != 0.0f) {
		float ndcX = clipPos.x / clipPos.w;
		float ndcY = clipPos.y / clipPos.w;
		float screenX = (ndcX + 1.0f) * 0.5f * WindowAPI::kClientWidth;
		float screenY = (1.0f - (ndcY + 1.0f) * 0.5f) * WindowAPI::kClientHeight;
		return {screenX, screenY};
	} else {
		return { -10000.0f, -10000.0f };
	}
}

// シングルトンインスタンスの取得
Camera* Camera::GetInstance() {
	if (instance == nullptr) {
		instance = new Camera;
	}
	return instance;
}
