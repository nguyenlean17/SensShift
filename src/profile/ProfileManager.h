#pragma once

#include "Profile.h"
#include "ConfigManager.h"
#include "TransformationPipeline.h"
#include <mutex>
#include <unordered_set>
#include <optional>
#include <atomic>

// Holds a snapshot of pipeline configuration computed under lock,
// to be applied (via Pipeline::Configure) AFTER releasing m_mutex.
struct PipelineConfigSnapshot {
    std::string name;
    double xMult = 1.0;
    double yMult = 1.0;
    double overallMult = 1.0;
    AccelerationSettings accel;
    TransformationOrder order = TransformationOrder::SensThenAccel;
    bool effectiveActive = false;
    bool emergencyDisabled = false;
};

class ProfileManager {
public:
    explicit ProfileManager(TransformationPipeline& pipeline);
    ~ProfileManager() = default;

    void Initialize(const AppConfig& config);

    // Profile CRUD
    std::vector<Profile> GetProfiles() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_profiles;
    }
    std::optional<Profile> GetProfileById(const std::string& id) const;
    void SetDefaultProfileId(const std::string& id);
    std::string GetDefaultProfileId() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_defaultProfileId;
    }

    bool AddProfile(const Profile& profile);
    bool UpdateProfile(const Profile& profile);
    bool DeleteProfile(const std::string& id);

    // Hotkey & App state events (return true if input event should be blocked)
    bool OnKeyPressed(uint32_t vkCode, uint32_t modifiers);
    bool OnKeyReleased(uint32_t vkCode, uint32_t modifiers);
    bool OnMouseButtonPressed(MouseButton button, uint32_t modifiers);
    bool OnMouseButtonReleased(MouseButton button, uint32_t modifiers);
    void OnForegroundAppChanged(const std::string& processName);

    // Emergency Failsafe Killswitch
    void ToggleEmergencyDisable();
    bool IsEmergencyDisabled() const { return m_atomicEmergencyDisabled.load(std::memory_order_acquire); }
    void SetEmergencyDisabled(bool disabled);

    // Active status query
    std::string GetActiveProfileName() const;
    bool IsModifierActive() const;

    // Checks whether an input event should be blocked from passing to other apps
    bool ShouldBlockInput(const Hotkey& hk) const;

    // Direct profile selection (e.g. from UI or tray)
    void SetManualBaseProfile(const std::string& profileId);

private:
    TransformationPipeline& m_pipeline;
    mutable std::mutex m_mutex;

    std::vector<Profile> m_profiles;
    std::string m_defaultProfileId = "profile_default";
    std::string m_currentBaseProfileId = "profile_default";
    std::unordered_map<std::string, std::string> m_appMappings;

    // Tracking active profiles
    std::unordered_set<std::string> m_activeHoldProfiles;
    std::unordered_set<std::string> m_activeToggleProfiles;
    std::string m_activeOverrideProfileId;

    bool m_emergencyDisabled = false;

    // Lock-free snapshots for the UI thread — updated inside RecomputeEffectiveProfile
    // so the HUD timer never needs to acquire m_mutex
    std::atomic<bool> m_atomicEmergencyDisabled{false};
    std::atomic<bool> m_atomicModifierActive{false};

    // Returns a snapshot to be applied via ApplyPipelineConfig() AFTER releasing m_mutex.
    // Caller MUST hold m_mutex when calling this.
    std::optional<PipelineConfigSnapshot> RecomputeEffectiveProfile();
    void ApplyPipelineConfig(const std::optional<PipelineConfigSnapshot>& snapshot);
    const Profile* FindProfilePtr(const std::string& id) const;
};
