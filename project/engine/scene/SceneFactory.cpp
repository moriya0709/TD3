#include "SceneFactory.h"

std::unique_ptr <BaseScene> SceneFactory::CreateScene(const std::string& sceneName) {
   // 次のシーンを生成
	std::unique_ptr <BaseScene> newScene = nullptr;

	if (sceneName == "TITLE") {
		newScene = std::make_unique <TitleScene>();
	} else if (sceneName == "GAMESELECT") {
		newScene = std::make_unique <StageSelect>();
	} else if (sceneName == "GAMEPLAY") {
		newScene = std::make_unique <GamePlayScene>();
		newScene->SetPlayerStyle(currentStyle);
		newScene->SetCurrentStage(currentStage);
	} else if (sceneName == "RESULT"){
		newScene = std::make_unique <resultScene>();
		newScene->SetPlayerStyle(currentStyle);
		newScene->SetCurrentStage(currentStage);
	}

	return newScene;
}
