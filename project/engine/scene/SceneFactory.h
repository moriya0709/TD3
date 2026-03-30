#pragma once
#include "AbstractSceneFactory.h"
#include "TitleScene.h"
#include "GamePlayScene.h"
#include"StageSelect.h"
#include "Player.h"

class SceneFactory : public AbstractSceneFactory{
public:
	void SetPlayerStyle(Player::Style style)override {
		 currentStyle=style;

	// シーン生成
	}
	std::unique_ptr <BaseScene> CreateScene(const std::string& sceneName) override;
private:
	Player::Style currentStyle;

};

