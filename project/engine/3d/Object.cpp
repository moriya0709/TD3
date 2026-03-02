#include "Object.h"
#include "Model.h"
#include "ModelManager.h"
#include "Camera.h"
#include "CameraManager.h"
#include "DirectXCommon.h"

void Object::Initialize(Camera* camera) {
	// 引数で受け取ってメンバ変数に記録する
	dxCommon_ = DirectXCommon::GetInstance();
	// デフォルトカメラをセット
	camera_ = camera;

	
	// *座標変換行列* //
	transformationMatrixResource = dxCommon_->CreateBufferResource(sizeof(TransformationMatrix));
	// 書き込む為のアドレスを取得
	transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));
	// 単位行列を書き込んでおく
	transformationMatrixData->WVP = MakeIdentity4x4();
	transformationMatrixData->World = MakeIdentity4x4();

	// *平行光源* //

	// リソース
	directionalLightResource = dxCommon_->CreateBufferResource(sizeof(DirectionalLight));
	// 書き込む
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
	// 初期値を書き込む
	directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	directionalLightData->direction = { 0.0f, -1.0f, 0.0f };
	directionalLightData->intensity = 1.0f;
	directionalLightData->isDisplay = false;
	directionalLightResource->Unmap(0, nullptr);

	// *環境光* //
	ambientLightResource = dxCommon_->CreateBufferResource(sizeof(AmbientLight));
	ambientLightResource->Map(0, nullptr, reinterpret_cast<void**>(&ambientLightData));
	// 初期値
	ambientLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	ambientLightData->intensity = 1.0f;
	ambientLightData->isDisplay = true;
	ambientLightResource->Unmap(0, nullptr);

	// *ポイントライト* //
	pointLightResource = dxCommon_->CreateBufferResource(sizeof(PointLight));
	pointLightResource->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData));
	// 初期値
	pointLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	pointLightData->position = { 1.0f, 1.0f, 1.0f};
	pointLightData->intensity = 1.0f;
	pointLightData->radius = 5.0f;
	pointLightData->isDisplay = false;
	pointLightResource->Unmap(0, nullptr);

	// *スポットライト* //
	spotLightResource = dxCommon_->CreateBufferResource(sizeof(SpotLight));
	spotLightResource->Map(0, nullptr, reinterpret_cast<void**>(&spotLightData));
	// 初期値
	spotLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	spotLightData->position = { 0.0f, 3.0f, 0.0f };
	spotLightData->intensity = 1.0f;
	spotLightData->direction = { 0.0f, 0.0f, 0.0f };
	spotLightData->range = 10.0f;
	spotLightData->innerCone = 1.0f;
	spotLightData->outerCone = 0.0f;
	spotLightData->isDisplay = false;
	spotLightResource->Unmap(0, nullptr);

	// アウトライン
	outlineResource = dxCommon_->CreateBufferResource(sizeof(Outline));
	outlineResource->Map(0, nullptr, reinterpret_cast<void**>(&outlineData));
	outlineData->thickness = 0.01f;
	outlineData->color = {1,0,0,0};

	// *Transform* //
	transform = {
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{0.0f,0.0f,0.0f}
	};
	cameraTransform = {
		{1.0f,1.0f,1.0f},
		{0.3f,0.0f,0.0f},
		{0.0f,4.0f,-10.0f}
	};

}

void Object::Update() {
	// Transformの更新
	camera_ = CameraManager::GetInstance()->GetActiveCamera();

	Matrix4x4 world =
		MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

	Matrix4x4 wvp =
		Multiply(world, camera_->GetViewProjectionMatrix());

	transformationMatrixData->WVP = wvp;
	transformationMatrixData->World = world;
}

void Object::Draw() {
	// wvp用とWorld用のCBufferの場所を設定
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource->GetGPUVirtualAddress());
	// アウトライン
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(3, outlineResource->GetGPUVirtualAddress());
	// 平行光
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(4, directionalLightResource->GetGPUVirtualAddress());
	// 環境光
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(5, ambientLightResource->GetGPUVirtualAddress());
	// ポイントライト
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(6, pointLightResource->GetGPUVirtualAddress());
	// スポットライト
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(7, spotLightResource->GetGPUVirtualAddress());

	// 3Dモデルが割り当てられていれば描画する
	if (model_) {
		model_->Draw();
	}

}

void Object::SetModel(const std::string& filePath) {
	// モデルを検索してセットする
	model_ = ModelManager::GetInstance()->FindModel(filePath);
}
