#include "Logger.h"

namespace Logger {

    std::ofstream logStream;

    void InitializeLog() {
        logStream.open("log.txt", std::ios::out | std::ios::trunc);
        if (!logStream.is_open()) {
            throw std::runtime_error("Failed to open log file.");
        }
    }

}