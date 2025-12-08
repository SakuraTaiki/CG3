#pragma once
#include <string>
#include <fstream>

namespace Logger {

    extern std::ofstream logStream;

    void Log(std::ofstream& stream, const std::string& message);

}
