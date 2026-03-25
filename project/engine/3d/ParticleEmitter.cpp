#include "ParticleEmitter.h"
#include "particleManager.h"
#include <iostream>

void ParticleEmitter::Initialize(std::string name, const Transform& transform, uint32_t count, float frequency) {
	emitter.transform = transform;
	emitter.count = count;
	emitter.frequency = frequency;
	emitter.frequencyTime = 0.0f;
	emitter.name = name;
}

void ParticleEmitter::Update() {
	// 時間経過によって発生させる
	emitter.frequencyTime += kDeltaTime; // 時刻を進める
	if (emitter.frequency <= emitter.frequencyTime) { // 頻度より大きいなら発生
		ParticleManager::GetInstance()->Emit(emitter.name, emitter.transform.translate, emitter.count);
		emitter.frequencyTime -= emitter.frequency; // 余計に過ぎた時間も紙して頻度計算する
	}
}

void ParticleEmitter::Emit() {
	ParticleManager::GetInstance()->Emit(emitter.name, emitter.transform.translate, emitter.count);

}

// アクティブ設定
void ParticleEmitter::SetActive(const std::string& name) {
	emitter.name = name;
}
