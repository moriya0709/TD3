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

	// DirectInput オブジェクト生成
	HRESULT hr;
	hr = DirectInput8Create(
		GetModuleHandle(nullptr),
		DIRECTINPUT_VERSION,
		IID_IDirectInput8,
		(void**)&directInput,
		nullptr
	);
	// マウスデバイス生成
	hr = directInput->CreateDevice(GUID_SysMouse, &mouse, nullptr);
	// データフォーマット設定
	hr = mouse->SetDataFormat(&c_dfDIMouse);
	// 取得開始
	mouse->Acquire();

	// --- ゲームパッドの列挙 ---
// 接続されているジョイスティック（パッド）を探して、見つかるたびに EnumJoysticksCallback を呼ぶ
	result = directInput->EnumDevices(
		DI8DEVCLASS_GAMECTRL,
		[](const DIDEVICEINSTANCE* pdidInstance, VOID* pContext) -> BOOL {
			auto self = static_cast<Input*>(pContext);
			ComPtr<IDirectInputDevice8> newPad;

			// デバイス生成
			if (FAILED(self->directInput->CreateDevice(pdidInstance->guidInstance, &newPad, nullptr))) {
				return DIENUM_CONTINUE;
			}

			// フォーマット設定
			newPad->SetDataFormat(&c_dfDIJoystick2);
			// 排他制御
			newPad->SetCooperativeLevel(self->windowAPI_->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

			// ★ここが重要！リストに追加
			self->gamepads.push_back(newPad);
			self->padStates.emplace_back(); // 状態保存用の箱も増やす

			return DIENUM_CONTINUE;
		},
		this, DIEDFL_ATTACHEDONLY);
}

// 更新
void Input::Update() {
	// 前回のキー入力を保存
	memcpy(keyPre, key, sizeof(key));

	// キーボード情報の取得開始
	keyboard->Acquire();
	// 全キーの入力情報を取得する
	keyboard->GetDeviceState(sizeof(key), key);

	// マウススクリーン座標取得
	POINT pos;
	GetCursorPos(&pos);
	ScreenToClient(windowAPI_->GetHwnd(), &pos);

	mouseScreenX = pos.x;
	mouseScreenY = pos.y;

	// マウスボタン
   // 現在の状態をクリアしてから取得
	ZeroMemory(&mouseState, sizeof(mouseState));

	HRESULT hr = mouse->GetDeviceState(sizeof(DIMOUSESTATE), &mouseState);
	if (FAILED(hr)) {
		// フォーカスが外れた場合は再取得を試みる
		mouse->Acquire();
		mouse->GetDeviceState(sizeof(DIMOUSESTATE), &mouseState);
	}

	// Update関数の最後に追加
	for (size_t i = 0; i < gamepads.size(); i++) {
		HRESULT hr = gamepads[i]->Poll(); // データを最新に更新
		if (FAILED(hr)) {
			gamepads[i]->Acquire();
			continue;
		}
		// 現在の状態を取得して padStates[i] に格納
		gamepads[i]->GetDeviceState(sizeof(DIJOYSTATE), &padStates[i]);
	}
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
	if (button < 0 || button >= 4) return false;
	if (mouseState.rgbButtons[button] & 0x80) {
		return true;
	}
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
LONG Input::GetPadLeftAxisX(int padIndex) {
	// 1. padIndexが負の数ではないか
	// 2. padIndexが現在のvectorのサイズ（中身の数）を超えていないか
	if (padIndex >= 0 && padIndex < static_cast<int>(padStates.size())) {
		return padStates[padIndex].lX;
	}

	// パッドが認識されていない場合は 0 を返して安全にやり過ごす
	return 0;
}
LONG Input::GetPadLeftAxisY(int padIndex) {
	// 1. padIndexが負の数ではないか
	// 2. padIndexが現在のvectorのサイズ（中身の数）を超えていないか
	if (padIndex >= 0 && padIndex < static_cast<int>(padStates.size())) {
		return padStates[padIndex].lY;
	}

	// パッドが認識されていない場合は 0 を返して安全にやり過ごす
	return 0;
}

LONG Input::GetPadRightAxisX(int padIndex) {
	if (padIndex >= 0 && padIndex < static_cast<int>(padStates.size())) {
		return padStates[padIndex].lZ; // 右Xは lZ
	}
	return 0;
}

LONG Input::GetPadRightAxisY(int padIndex) {
	if (padIndex >= 0 && padIndex < static_cast<int>(padStates.size())) {
		return padStates[padIndex].lRz; // 右Yは lRz
	}
	return 0;
}

