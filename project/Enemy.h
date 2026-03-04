#pragma once
#include "BaseScene.h"
#include "Camera.h"
#include "CameraManager.h"
#include "ImGuiManager.h"
#include "Input.h"
#include "ModelManager.h"
#include "Object.h"
#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include "PostEffect.h"
#include "SoundManager.h"
#include "Sprite.h"

class Enemy {
public:

    /// <summary>
    /// ‰Šú‰»
    /// </summary>
    /// <param name="camera"></param>
    virtual void Initialize(Camera* camera);

    /// <summary>
    /// XV
    /// </summary>
    virtual void Update();

    /// <summary>
    /// •`‰æ
    /// </summary>
    virtual void Draw3D();

private:
    //Transform transform_; // À•WŒn
    //float activeTimer; // ‘¶İ‚·‚éŠÔ
    //float isAvile; // ¶‘¶‚µ‚Ä‚¢‚é‚©
    //float health; // ‘Ì—Í

    //Camera* camera_ = nullptr;

};
