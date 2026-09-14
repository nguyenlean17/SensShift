#include "ProfileManager.h"
#include <algorithm>

ProfileManager::ProfileManager(TransformationPipeline& pipeline)
    : m_pipeline(pipeline) {}

void ProfileManager::ApplyPipelineConfig(const std::optional<PipelineConfigSnapshot>& snapshot) {
    if (!snapshot) return;
    const auto& s = *snapshot;
    m_pipeline.Configure(
        s.name,
        s.xMult,
        s.yMult,
        s.overallMult,
        s.accel,
        s.order,
        s.effectiveActive,
        s.emergencyDisabled
    );
}

void ProfileManager::Initialize(const AppConfig& config) {
    std::optional<PipelineConfigSnapshot> snap;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_profiles = config.profiles;
        m_defaultProfileId = config.defaultProfileId;
        m_currentBaseProfileId = config.defaultProfileId;
        m_appMappings = config.appMappings;
        m_emergencyDisabled = config.emergencyDisable;
        m_atomicEmergencyDisabled.store(config.emergencyDisable, std::memory_order_release);
        m_activeHoldProfiles.clear();
        m_activeToggleProfiles.clear();
        m_activeOverrideProfileId.clear();

        snap = RecomputeEffectiveProfile();
    }
    ApplyPipelineConfig(snap);
}

std::optional<Profile> ProfileManager::GetProfileById(const std::string& id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    const Profile* p = FindProfilePtr(id);
    if (p) return *p;
    return std::nullopt;
}

const Profile* ProfileManager::FindProfilePtr(const std::string& id) const {
    for (const auto& p : m_profiles) {
        if (p.id == id) return &p;
    }
    return nullptr;
}

void ProfileManager::SetDefaultProfileId(const std::string& id) {
    std::optional<PipelineConfigSnapshot> snap;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_defaultProfileId = id;
        snap = RecomputeEffectiveProfile();
    }
    ApplyPipelineConfig(snap);
}

bool ProfileManager::AddProfile(const Profile& profile) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& p : m_profiles) {
        if (p.id == profile.id) return false;
    }
    m_profiles.push_back(profile);
    return true;
}

bool ProfileManager::UpdateProfile(const Profile& profile) {
    std::optional<PipelineConfigSnapshot> snap;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& p : m_profiles) {
            if (p.id == profile.id) {
                p = profile;
                snap = RecomputeEffectiveProfile();
                break;
            }
        }
        if (!snap) return false;
    }
    ApplyPipelineConfig(snap);
    return true;
}

bool ProfileManager::DeleteProfile(const std::string& id) {
    std::optional<PipelineConfigSnapshot> snap;
    bool deleted = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_profiles.size() <= 1) return false; // Prevent deleting last profile

        auto it = std::remove_if(m_profiles.begin(), m_profiles.end(),
            [&](const Profile& p) { return p.id == id; });
        if (it != m_profiles.end()) {
            m_profiles.erase(it, m_profiles.end());
            m_activeHoldProfiles.erase(id);
            m_activeToggleProfiles.erase(id);
            if (m_activeOverrideProfileId == id) m_activeOverrideProfileId.clear();
            if (m_defaultProfileId == id) m_defaultProfileId = m_profiles.front().id;
            if (m_currentBaseProfileId == id) m_currentBaseProfileId = m_defaultProfileId;
            snap = RecomputeEffectiveProfile();
            deleted = true;
        }
    }
    if (deleted) ApplyPipelineConfig(snap);
    return deleted;
}

void ProfileManager::SetManualBaseProfile(const std::string& profileId) {
    std::optional<PipelineConfigSnapshot> snap;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (FindProfilePtr(profileId)) {
            m_currentBaseProfileId = profileId;
            snap = RecomputeEffectiveProfile();
        }
    }
    ApplyPipelineConfig(snap);
}

bool ProfileManager::OnKeyPressed(uint32_t vkCode, uint32_t modifiers) {
    std::optional<PipelineConfigSnapshot> snap;
    bool shouldBlock = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Emergency Failsafe check
        if (IsEmergencyFailsafeCombo(vkCode, modifiers)) {
            m_emergencyDisabled = !m_emergencyDisabled;
            m_atomicEmergencyDisabled.store(m_emergencyDisabled, std::memory_order_release);
            snap = RecomputeEffectiveProfile();
            shouldBlock = true; // Consume failsafe hotkey
        } else {
            if (m_emergencyDisabled) return false;

            bool matched = false;
            for (const auto& p : m_profiles) {
                if (p.hotkey.vkCode == vkCode && (p.hotkey.modifiers == modifiers || (p.hotkey.modifiers == ModNone && (vkCode == VK_SHIFT || vkCode == VK_CONTROL || vkCode == VK_MENU)))) {
                    matched = true;
                    if (p.blockOriginalHotkeyInput) {
                        shouldBlock = true;
                    }
                    if (p.activationMode == ActivationMode::Hold) {
                        m_activeHoldProfiles.insert(p.id);
                    } else if (p.activationMode == ActivationMode::HoldOverride) {
                        m_activeOverrideProfileId = p.id;
                    } else if (p.activationMode == ActivationMode::Toggle) {
                        if (m_activeToggleProfiles.contains(p.id)) {
                            m_activeToggleProfiles.erase(p.id);
                        } else {
                            m_activeToggleProfiles.insert(p.id);
                        }
                    }
                }
            }

            // ONLY recompute if a profile actually matched
            if (matched) {
                snap = RecomputeEffectiveProfile();
            }
        }
    }
    ApplyPipelineConfig(snap);
    return shouldBlock;
}

bool ProfileManager::OnKeyReleased(uint32_t vkCode, uint32_t /*modifiers*/) {
    std::optional<PipelineConfigSnapshot> snap;
    bool shouldBlock = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_emergencyDisabled) return false;

        bool changed = false;
        for (const auto& p : m_profiles) {
            if (p.hotkey.vkCode == vkCode) {
                if (p.blockOriginalHotkeyInput) {
                    shouldBlock = true;
                }
                if (p.activationMode == ActivationMode::Hold) {
                    if (m_activeHoldProfiles.erase(p.id) > 0) {
                        changed = true;
                    }
                } else if (p.activationMode == ActivationMode::HoldOverride) {
                    if (m_activeOverrideProfileId == p.id) {
                        m_activeOverrideProfileId.clear();
                        changed = true;
                    }
                }
            }
        }

        // ONLY recompute if an active hold profile was released
        if (changed) {
            snap = RecomputeEffectiveProfile();
        }
    }
    ApplyPipelineConfig(snap);
    return shouldBlock;
}

bool ProfileManager::OnMouseButtonPressed(MouseButton button, uint32_t modifiers) {
    std::optional<PipelineConfigSnapshot> snap;
    bool shouldBlock = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_emergencyDisabled) return false;

        bool matched = false;
        for (const auto& p : m_profiles) {
            if (p.hotkey.mouseButton == button && (p.hotkey.modifiers == modifiers || p.hotkey.modifiers == ModNone)) {
                matched = true;
                if (p.blockOriginalHotkeyInput) {
                    shouldBlock = true;
                }
                if (p.activationMode == ActivationMode::Hold) {
                    m_activeHoldProfiles.insert(p.id);
                } else if (p.activationMode == ActivationMode::HoldOverride) {
                    m_activeOverrideProfileId = p.id;
                } else if (p.activationMode == ActivationMode::Toggle) {
                    if (m_activeToggleProfiles.contains(p.id)) {
                        m_activeToggleProfiles.erase(p.id);
                    } else {
                        m_activeToggleProfiles.insert(p.id);
                    }
                }
            }
        }

        // ONLY recompute if a profile hotkey matched
        if (matched) {
            snap = RecomputeEffectiveProfile();
        }
    }
    ApplyPipelineConfig(snap);
    return shouldBlock;
}

bool ProfileManager::OnMouseButtonReleased(MouseButton button, uint32_t /*modifiers*/) {
    std::optional<PipelineConfigSnapshot> snap;
    bool shouldBlock = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_emergencyDisabled) return false;

        bool changed = false;
        for (const auto& p : m_profiles) {
            if (p.hotkey.mouseButton == button) {
                if (p.blockOriginalHotkeyInput) {
                    shouldBlock = true;
                }
                if (p.activationMode == ActivationMode::Hold) {
                    if (m_activeHoldProfiles.erase(p.id) > 0) {
                        changed = true;
                    }
                } else if (p.activationMode == ActivationMode::HoldOverride) {
                    if (m_activeOverrideProfileId == p.id) {
                        m_activeOverrideProfileId.clear();
                        changed = true;
                    }
                }
            }
        }

        // ONLY recompute if an active hold profile was released
        if (changed) {
            snap = RecomputeEffectiveProfile();
        }
    }
    ApplyPipelineConfig(snap);
    return shouldBlock;
}

void ProfileManager::OnForegroundAppChanged(const std::string& processName) {
    std::optional<PipelineConfigSnapshot> snap;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_appMappings.find(processName);
        if (it != m_appMappings.end()) {
            if (FindProfilePtr(it->second)) {
                m_currentBaseProfileId = it->second;
                snap = RecomputeEffectiveProfile();
            }
        }
        if (!snap) {
            // Revert to default profile if no specific mapping
            m_currentBaseProfileId = m_defaultProfileId;
            snap = RecomputeEffectiveProfile();
        }
    }
    ApplyPipelineConfig(snap);
}

void ProfileManager::ToggleEmergencyDisable() {
    std::optional<PipelineConfigSnapshot> snap;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_emergencyDisabled = !m_emergencyDisabled;
        m_atomicEmergencyDisabled.store(m_emergencyDisabled, std::memory_order_release);
        snap = RecomputeEffectiveProfile();
    }
    ApplyPipelineConfig(snap);
}

void ProfileManager::SetEmergencyDisabled(bool disabled) {
    std::optional<PipelineConfigSnapshot> snap;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_emergencyDisabled = disabled;
        m_atomicEmergencyDisabled.store(disabled, std::memory_order_release);
        snap = RecomputeEffectiveProfile();
    }
    ApplyPipelineConfig(snap);
}

bool ProfileManager::ShouldBlockInput(const Hotkey& hk) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& p : m_profiles) {
        if (p.hotkey == hk && p.blockOriginalHotkeyInput) {
            return true;
        }
    }
    return false;
}

std::string ProfileManager::GetActiveProfileName() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    const Profile* p = nullptr;
    if (!m_activeOverrideProfileId.empty()) {
        p = FindProfilePtr(m_activeOverrideProfileId);
    } else if (!m_activeHoldProfiles.empty()) {
        p = FindProfilePtr(*m_activeHoldProfiles.begin());
    } else if (!m_activeToggleProfiles.empty()) {
        p = FindProfilePtr(*m_activeToggleProfiles.begin());
    } else {
        p = FindProfilePtr(m_currentBaseProfileId);
    }
    return p ? p->name : "Default";
}

bool ProfileManager::IsModifierActive() const {
    return m_atomicModifierActive.load(std::memory_order_acquire);
}

std::optional<PipelineConfigSnapshot> ProfileManager::RecomputeEffectiveProfile() {
    // NOTE: m_mutex MUST be held by the caller.
    // This function computes the pipeline params and returns a snapshot.
    // The caller applies the snapshot AFTER releasing m_mutex,
    // breaking the ABBA deadlock cycle between m_mutex and Pipeline::m_mutex.

    const Profile* active = nullptr;

    if (!m_activeOverrideProfileId.empty()) {
        active = FindProfilePtr(m_activeOverrideProfileId);
    } else if (!m_activeHoldProfiles.empty()) {
        active = FindProfilePtr(*m_activeHoldProfiles.begin());
    } else if (!m_activeToggleProfiles.empty()) {
        active = FindProfilePtr(*m_activeToggleProfiles.begin());
    } else {
        active = FindProfilePtr(m_currentBaseProfileId);
        if (!active) active = FindProfilePtr(m_defaultProfileId);
        if (!active && !m_profiles.empty()) active = &m_profiles.front();
    }

    if (!active) {
        return std::nullopt;
    }

    bool isModifierActive = (!m_activeOverrideProfileId.empty() ||
                             !m_activeHoldProfiles.empty() ||
                             !m_activeToggleProfiles.empty());

    // If the base profile itself has custom sensitivity or acceleration, it is considered active
    bool baseHasTransformation = (active->xMultiplier != 1.0 ||
                                  active->yMultiplier != 1.0 ||
                                  active->overallMultiplier != 1.0 ||
                                  active->acceleration.enabled);

    bool effectiveActive = (isModifierActive || baseHasTransformation) && !m_emergencyDisabled;

    // Update atomic snapshots for lock-free reads by the UI thread
    m_atomicModifierActive.store(
        isModifierActive && !m_emergencyDisabled, std::memory_order_release);
    m_atomicEmergencyDisabled.store(m_emergencyDisabled, std::memory_order_release);

    // Build snapshot — caller will apply after releasing lock
    PipelineConfigSnapshot snap;
    snap.name = active->name;
    snap.xMult = active->xMultiplier;
    snap.yMult = active->yMultiplier;
    snap.overallMult = active->overallMultiplier;
    snap.accel = active->acceleration;
    snap.order = active->order;
    snap.effectiveActive = effectiveActive;
    snap.emergencyDisabled = m_emergencyDisabled;

    return snap;
}
