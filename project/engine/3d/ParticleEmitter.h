#pragma once
#include <cstdint>
#include <string>
#include <cassert>

#include "Calc.h"

struct Emitter {
	Transform transform; //!< エミッタのTransform
	uint32_t count; //!< 発生数
	float frequency; //!< 発生頻度
	float frequencyTime; //!< 頻度用時刻
	std::string name; //!< エミッタ名
};


class ParticleEmitter {
public:
	// 初期化
	void Initialize(std::string name, const Transform& transform, uint32_t count, float frequency);
	// 更新
	void Update();

	// パーティクル発生
	void Emit();
	// アクティブ設定
	void SetActive(const std::string& name);

	//���W�X�V
	void SetTranslate(const Vector3& translate)
	{
		emitter.transform.translate = translate;
	}

private:
	// パーティクルエミッタの設定
	Emitter emitter;

	// デルタタイム(60fps固定)
	const float kDeltaTime = 1.0f / 60.0f;
	
};