#pragma once

#include <Windows.h>
#include <cassert>
#include <vector>
#include <dinput.h>
#include <wrl.h>

#include "WindowAPI.h"


class Input {
public:
	// namespace省略
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

	//初期化
	void Initialize(WindowAPI* windowAPI);
	// 更新
	void  Update();

	// シングルトンインスタンスの取得
	static Input* GetInstance();

	// キーが押されたかどうかを調べる
	bool PushKey(BYTE keyNumBer); // プッシュ
	bool TriggerKey(BYTE keyNumber); // トリガー

	// マウス
	LONG GetMouseX() const { return mouseState.lX; }
	LONG GetMouseY() const { return mouseState.lY; }
	bool IsMouseButtonPressed(int button);

	// ゲームパッド
	bool IsPadButtonPressed(int padIndex, int button);
	LONG GetPadAxisX(int padIndex);
	LONG GetPadAxisY(int padIndex);

private:
	// DirectInputのインスタンス
	ComPtr<IDirectInput8> directInput = nullptr;

	// キーボード
	ComPtr<IDirectInputDevice8> keyboard = nullptr;
	BYTE key[256] = {}; // 現在のキー状態
	BYTE keyPre[256] = {}; // 前フレームのキー状態

	// マウス
	ComPtr<IDirectInputDevice8> mouse = nullptr;
	DIMOUSESTATE mouseState{}; // マウスの状態

	// ゲームパッド
	std::vector<IDirectInputDevice8> gamepads;
	std::vector<DIJOYSTATE> padStates;

	// シングルトンインスタンス
	static std::unique_ptr <Input> instance;

	// WindowAPI
	WindowAPI* windowAPI_ = nullptr;

};

