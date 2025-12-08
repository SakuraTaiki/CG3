#pragma once
#include <string>
#include <fstream>

namespace Logger {

    extern std::ofstream logStream;

    void Log(std::ostream& os, const std::string& message);

}
