#include "ConfigManager.h"
#include <fstream>
#include <shlobj.h>

ConfigManager::ConfigManager() {
    m_currentPath = GetDefaultConfigPath();
    PopulateDefaults();
}

std::wstring ConfigManager::GetDefaultConfigPath() const {
    WCHAR path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        std::wstring dir = std::wstring(path) + L"\\MouseSensitivityModifier";
        CreateDirectoryW(dir.c_str(), NULL);
        return dir + L"\\config.json";
    }
    return L"config.json";
}

void ConfigManager::PopulateDefaults() {
    m_config.defaultProfileId = "profile_spray_control";
    m_config.profiles.clear();
    m_config.profiles.push_back(Profile::CreateDefault());
    m_config.profiles.push_back(Profile::CreatePrecision());
    m_config.profiles.push_back(Profile::CreateSprayControl());
    m_config.profiles.push_back(Profile::CreateFastTurn());

    m_config.appMappings.clear();
    m_config.appMappings["PUBG"] = "profile_spray_control";
    m_config.appMappings["TslGame.exe"] = "profile_spray_control";
    m_config.appMappings["game.exe"] = "profile_fast_turn";
}

bool ConfigManager::Load(const std::wstring& configPath) {
    std::wstring path = configPath.empty() ? m_currentPath : configPath;
    std::ifstream file(path);
    if (!file.is_open()) {
        PopulateDefaults();
        Save(path);
        return false;
    }

    try {
        nlohmann::json j;
        file >> j;

        m_config.defaultProfileId = j.value("defaultProfileId", "profile_default");
        m_config.startMinimized = j.value("startMinimized", false);

        if (j.contains("profiles") && j["profiles"].is_array()) {
            m_config.profiles.clear();
            for (const auto& pj : j["profiles"]) {
                m_config.profiles.push_back(Profile::FromJson(pj));
            }
        }

        if (j.contains("appMappings") && j["appMappings"].is_object()) {
            m_config.appMappings.clear();
            for (auto& [app, profId] : j["appMappings"].items()) {
                m_config.appMappings[app] = profId.get<std::string>();
            }
        }

        bool hasSpray = false;
        for (const auto& p : m_config.profiles) {
            if (p.id == "profile_spray_control") {
                hasSpray = true;
                break;
            }
        }
        if (!hasSpray) {
            m_config.profiles.insert(m_config.profiles.begin(), Profile::CreateSprayControl());
        }
        m_config.defaultProfileId = "profile_spray_control";
        m_config.appMappings["PUBG"] = "profile_spray_control";
        m_config.appMappings["TslGame.exe"] = "profile_spray_control";

        if (m_config.profiles.empty()) {
            PopulateDefaults();
        }

        return true;
    } catch (...) {
        PopulateDefaults();
        return false;
    }
}

bool ConfigManager::Save(const std::wstring& configPath) {
    std::wstring path = configPath.empty() ? m_currentPath : configPath;
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }

    try {
        nlohmann::json j;
        j["defaultProfileId"] = m_config.defaultProfileId;
        j["startMinimized"] = m_config.startMinimized;

        nlohmann::json profArray = nlohmann::json::array();
        for (const auto& p : m_config.profiles) {
            profArray.push_back(p.ToJson());
        }
        j["profiles"] = profArray;

        nlohmann::json mapObj = nlohmann::json::object();
        for (const auto& [app, profId] : m_config.appMappings) {
            mapObj[app] = profId;
        }
        j["appMappings"] = mapObj;

        file << j.dump(4);
        return true;
    } catch (...) {
        return false;
    }
}
