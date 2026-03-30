#pragma once
#include "BaseScene.h"

class SelectScene : public BaseScene {
public:
	// ‰Šú‰»
	void Initialize() override;
	// XV
	void Update() override;
	// •`‰æ
	void Draw2D() override;
	void Draw3D() override;
	// I—¹
	void Finalize() override;

};

