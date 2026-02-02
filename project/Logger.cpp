#include "Logger.h"

#include <format>
// ログ出力のための名前空間
namespace Logger {

	void Log(const std::string& message) {
		OutputDebugStringA(message.c_str());
	}
}