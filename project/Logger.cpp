#include "Logger.h"
#include <Windows.h>

namespace Logger {

    std::ofstream logStream;

    void InitializeLog() {
        logStream.open("log.txt", std::ios::out | std::ios::trunc);
        if (!logStream.is_open()) {
            throw std::runtime_error("Failed to open log file.");
        }
    }

    void Log(std::ostream& os, const std::string& message)
    {
        os << message << std::endl;
        OutputDebugStringA(message.c_str());

    }

}