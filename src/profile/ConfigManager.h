#pragma once

#include "Profile.h"
#include <string>
#include <vector>
#include <unordered_map>

struct AppConfig {
    std::string defaultProfileId = "profile_default";
    std::vector<Profile> profiles;
    std::unordered_map<std::string, std::string> appMappings; // processName -> profileId
    bool startMinimized = false;
    bool emergencyDisable = false;
};

class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager() = default;

    bool Load(const std::wstring& configPath = L"");
    bool Save(const std::wstring& configPath = L"");

    const AppConfig& GetConfig() const { return m_config; }
    AppConfig& GetConfig() { return m_config; }

    std::wstring GetDefaultConfigPath() const;

private:
    AppConfig m_config;
    std::wstring m_currentPath;

    void PopulateDefaults();
};
