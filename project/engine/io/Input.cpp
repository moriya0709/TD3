#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include "Input.h"

#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")

std::unique_ptr <Input> Input::instance = nullptr;

// 初期化
void Input::Initialize(WindowAPI* windowAPI) {
	// 借りてきたWindowAPIのインスタンスを記録
	this->windowAPI_ = windowAPI;

	HRESULT result;

	// DirectInputの初期化
	result = DirectInput8Create(
		windowAPI_->GetHInstance(), DIRECTINPUT_VERSION, IID_IDirectInput8,
		(void**)&directInput, nullptr);
	assert(SUCCEEDED(result));

	// キーボードデバイスの生成
	result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(result));

	// 入力データ形式のセット
	result = keyboard->SetDataFormat(&c_dfDIKeyboard); // 標準形式
	assert(SUCCEEDED(result));

	// 排他制御レベルのセット
	result = keyboard->SetCooperativeLevel(
		windowAPI_->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(result));
}

// 更新
void Input::Update() {
	// 前回のキー入力を保存
	memcpy(keyPre, key, sizeof(key));

	// キーボード情報の取得開始
	keyboard->Acquire();
	// 全キーの入力情報を取得する
	keyboard->GetDeviceState(sizeof(key), key);

	// スクリーン座標取得
	POINT pos;
	GetCursorPos(&pos);
	ScreenToClient(windowAPI_->GetHwnd(), &pos);

	mouseScreenX = pos.x;
	mouseScreenY = pos.y;
}

Input* Input::GetInstance() {
	if (instance == nullptr) {
		instance = std::make_unique <Input>();
	}
	return instance.get();
}

// プッシュ入力
bool Input::PushKey(BYTE keyNumBer) {
	// 指定キーを押していればtrueを返す
	if (key[keyNumBer]) {
		return true;
	}

	// 押していなければfalseを返す
	return false;
}
// トリガー入力
bool Input::TriggerKey(BYTE keyNumber) {
	// 指定キーが押されていて、前回は押されていなければtrueを返す
	if (key[keyNumber] && !keyPre[keyNumber]) {
		return true;
	}

	return false;
}

// マウスボタンが押されたかどうか
bool Input::IsMouseButtonPressed(int button) {
	// 指定ボタン押していればtrueを返す
	if (mouseState.rgbButtons[button]) {
		return true;
	}

	// 押していなければfalseを返す
	return false;
}

// ゲームパッドのボタンが押されたかどうか
bool Input::IsPadButtonPressed(int padIndex, int button) {
	if (padIndex >= padStates.size()) return false;
	
	if (padStates[padIndex].rgbButtons[button]) {
		return true;
	}

	return false;
}

// ゲームパッドの軸の値を取得
LONG Input::GetPadAxisX(int padIndex) {
	return padStates[padIndex].lX; // 左スティックX
}
LONG Input::GetPadAxisY(int padIndex) {
	return padStates[padIndex].lY; // 左スティックY
}

