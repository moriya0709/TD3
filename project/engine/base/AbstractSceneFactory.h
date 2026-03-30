#pragma once
#include <string>
#include <memory>
#include "Player.h"
#include "BaseScene.h"

class AbstractSceneFactory {
public:
	// 仮想デストラクタ
	virtual ~AbstractSceneFactory() = default;

	virtual void SetPlayerStyle(Player::Style style);
	// シーン生成
	virtual std::unique_ptr <BaseScene> CreateScene(const std::string& sceneNama) = 0;




};

