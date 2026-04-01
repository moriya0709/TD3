#pragma once
#include "AbstractSceneFactory.h"
#include "GamePlayScene.h"
#include "StageSelect.h"
#include "TitleScene.h"

class SceneFactory : public AbstractSceneFactory {
public:
	void SetPlayerStyle(int style) override {
		currentStyle = style;

		// シーン生成
	}
	std::unique_ptr<BaseScene> CreateScene(const std::string& sceneName) override;

private:
	int currentStyle;
};
