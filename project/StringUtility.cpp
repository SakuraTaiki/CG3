#include "StringUtility.h"
#include <Windows.h>

namespace StringUtility
{
    // std::string → std::wstring
    std::wstring ConvertString(const std::string& str)
    {
        int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        std::wstring result(sizeNeeded, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], sizeNeeded);
        result.pop_back(); // null終端を削除
        return result;
    }

    // std::wstring → std::string
    std::string ConvertString(const std::wstring& str)
    {
        int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, nullptr, 0, 0, 0);
        std::string result(sizeNeeded, 0);
        WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, &result[0], sizeNeeded, 0, 0);
        result.pop_back();
        return result;
    }
}