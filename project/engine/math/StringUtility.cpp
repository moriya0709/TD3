#include "StringUtility.h"

namespace StringUtility {
	// string‚ðwstring‚É•ÏŠ·‚·‚é
	std::wstring ConvertString(const std::string& str) {
		if (str.empty()) return std::wstring();

		int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0);
		if (sizeNeeded == 0) return std::wstring();

		std::wstring result(sizeNeeded, 0);
		MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &result[0], sizeNeeded);
		return result;
	}

	// wstring‚ðstring‚É•ÏŠ·‚·‚é
	std::string ConvertString(const std::wstring& str) {
		if (str.empty()) {
			return std::string();
		}

		auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
		if (sizeNeeded == 0) {
			return std::string();
		}
		std::string result(sizeNeeded, 0);
		WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
		return result;
	}
}
