#pragma once
#include <DirectXMath.h>

#include "Camera.h"
#include "CameraManager.h"
#include "Sprite.h"
#include "Object.h"
#include "ParticleEmitter.h"
#include "CameraManager.h"
#include "ParticleManager.h"
#include "ModelManager.h"
#include "SoundManager.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "BaseScene.h"
#include "PostEffect.h"
#include "RailCamera.h"

using namespace DirectX;

class SpriteCommon;
class ObjectCommon;

class StageSelect : public BaseScene
{
public:

	//乗り物
	enum Style {
		normal,
		speed,
		power,
		sniper
	};

	void Initialize()override;
	void Update()override;
	void Draw2D()override;
	void Draw3D()override;

	void Finalize()override;

private:

	// プレイヤーの3Dオブジェクト
	std::unique_ptr<Object> playerObject_;

	Transform cameraTransform{
	   { 1.0f, 1.0f, 1.0f }, // scale
	   { 0.0f, 0.0f, 0.0f }, // rotate
	   { 0.0f, 0.0f, -5.0f } // translate
	};
	// パーティクル
	Transform transformParticle
	{
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{0.0f,0.0f,0.0f}
	};

	// カメラ
	std::unique_ptr<Camera> camera = nullptr;
	// スプライト
	std::unique_ptr <Sprite> sprite = nullptr;
	// 3Dオブジェクト
	std::unique_ptr <Object> object[2]{};
	// パーティクルエミッタ
	std::unique_ptr <ParticleEmitter> particleEmitter = nullptr;

	Style currentStyle = normal;

};

