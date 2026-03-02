#pragma once
#include "AbstractSceneFactory.h"
#include "TitleScene.h"
#include "GamePlayScene.h"

class SceneFactory : public AbstractSceneFactory{
public:
	// ÉVÅ[Éìê∂ê¨
	std::unique_ptr <BaseScene> CreateScene(const std::string& sceneName) override;

};

