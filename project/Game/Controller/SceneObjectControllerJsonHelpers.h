namespace {

    std::string EscapeJsonString(const std::string& value)
    {
        std::string result;

        for (char c : value) {
            if (c == '\\') {
                result += "\\\\";
            } else if (c == '"') {
                result += "\\\"";
            } else {
                result += c;
            }
        }

        return result;
    }

    bool SplitModelPath(
        const std::string& path,
        std::string& directoryPath,
        std::string& modelName
    )
    {
        const size_t slashPos =
            path.find_last_of("/\\");

        if (slashPos == std::string::npos) {
            directoryPath = "Resources";
            modelName = path;
            return true;
        }

        directoryPath =
            path.substr(0, slashPos);

        modelName =
            path.substr(slashPos + 1);

        return true;
    }

    std::string ExtractJsonString(
        const std::string& text,
        const std::string& key
    )
    {
        const std::regex pattern(
            "\"" + key + "\"\\s*:\\s*\"([^\"]*)\""
        );

        std::smatch match;

        if (std::regex_search(text, match, pattern)) {
            return match[1].str();
        }

        return "";
    }

    bool ExtractJsonBool(
        const std::string& text,
        const std::string& key,
        bool defaultValue
    )
    {
        const std::regex pattern(
            "\"" + key + "\"\\s*:\\s*(true|false)"
        );

        std::smatch match;

        if (std::regex_search(text, match, pattern)) {
            return match[1].str() == "true";
        }

        return defaultValue;
    }

    Vector3 ExtractJsonVector3(
        const std::string& text,
        const std::string& key,
        const Vector3& defaultValue
    )
    {
        const std::regex pattern(
            "\"" + key + "\"\\s*:\\s*\\[\\s*"
            "([-+0-9.eE]+)\\s*,\\s*"
            "([-+0-9.eE]+)\\s*,\\s*"
            "([-+0-9.eE]+)\\s*\\]"
        );

        std::smatch match;

        if (!std::regex_search(text, match, pattern)) {
            return defaultValue;
        }

        return {
            std::stof(match[1].str()),
            std::stof(match[2].str()),
            std::stof(match[3].str())
        };
    }

} // namespace

