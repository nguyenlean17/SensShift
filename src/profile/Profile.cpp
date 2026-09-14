#include "Profile.h"

nlohmann::json Profile::ToJson() const {
    nlohmann::json j;
    j["id"] = id;
    j["name"] = name;
    j["enabled"] = enabled;
    j["targetApp"] = targetApp;
    j["hotkey"] = {
        {"vkCode", hotkey.vkCode},
        {"mouseButton", static_cast<int>(hotkey.mouseButton)},
        {"modifiers", hotkey.modifiers}
    };
    j["activationMode"] = static_cast<int>(activationMode);
    j["activationModeStr"] = activationModeStr;
    j["blockOriginalHotkeyInput"] = blockOriginalHotkeyInput;

    nlohmann::json conds = nlohmann::json::array();
    for (const auto& c : conditions) {
        conds.push_back({ {"type", c.type}, {"text", c.text} });
    }
    j["conditions"] = conds;

    j["xMultiplier"] = xMultiplier;
    j["yMultiplier"] = yMultiplier;
    j["overallMultiplier"] = overallMultiplier;
    j["order"] = static_cast<int>(order);

    nlohmann::json acc;
    acc["enabled"] = acceleration.enabled;
    acc["type"] = static_cast<int>(acceleration.type);
    acc["strength"] = acceleration.strength;
    acc["threshold"] = acceleration.threshold;
    acc["maxMultiplier"] = acceleration.maxMultiplier;
    acc["exponent"] = acceleration.exponent;
    acc["minSpeed"] = acceleration.minSpeed;
    acc["maxSpeed"] = acceleration.maxSpeed;
    acc["sensMultiplier"] = acceleration.sensMultiplier;

    nlohmann::json pts = nlohmann::json::array();
    for (const auto& pt : acceleration.customPoints) {
        pts.push_back({{"speed", pt.first}, {"multiplier", pt.second}});
    }
    acc["customPoints"] = pts;
    j["acceleration"] = acc;

    return j;
}

Profile Profile::FromJson(const nlohmann::json& j) {
    Profile p;
    p.id = j.value("id", "profile_default");
    p.name = j.value("name", "Unnamed Profile");
    p.enabled = j.value("enabled", true);
    p.targetApp = j.value("targetApp", "PUBG");

    if (j.contains("hotkey") && j["hotkey"].is_object()) {
        const auto& hk = j["hotkey"];
        p.hotkey.vkCode = hk.value("vkCode", 0u);
        p.hotkey.mouseButton = static_cast<MouseButton>(hk.value("mouseButton", 0));
        p.hotkey.modifiers = hk.value("modifiers", 0u);
    }

    p.activationMode = static_cast<ActivationMode>(j.value("activationMode", 0));
    p.activationModeStr = j.value("activationModeStr", "While conditions are true");
    p.blockOriginalHotkeyInput = j.value("blockOriginalHotkeyInput", false);

    if (j.contains("conditions") && j["conditions"].is_array()) {
        p.conditions.clear();
        for (const auto& cj : j["conditions"]) {
            if (cj.is_object()) {
                p.conditions.push_back({
                    cj.value("type", "Mouse button held"),
                    cj.value("text", "")
                });
            }
        }
    }

    p.xMultiplier = j.value("xMultiplier", 1.0);
    p.yMultiplier = j.value("yMultiplier", 1.0);
    p.overallMultiplier = j.value("overallMultiplier", 1.0);
    p.order = static_cast<TransformationOrder>(j.value("order", 0));

    if (j.contains("acceleration") && j["acceleration"].is_object()) {
        const auto& acc = j["acceleration"];
        p.acceleration.enabled = acc.value("enabled", false);
        p.acceleration.type = static_cast<CurveType>(acc.value("type", 0));
        p.acceleration.strength = acc.value("strength", 0.5);
        p.acceleration.threshold = acc.value("threshold", 50.0);
        p.acceleration.maxMultiplier = acc.value("maxMultiplier", 2.5);
        p.acceleration.exponent = acc.value("exponent", 2.0);
        p.acceleration.minSpeed = acc.value("minSpeed", 0.0);
        p.acceleration.maxSpeed = acc.value("maxSpeed", 3000.0);
        p.acceleration.sensMultiplier = acc.value("sensMultiplier", 1.0);

        if (acc.contains("customPoints") && acc["customPoints"].is_array()) {
            p.acceleration.customPoints.clear();
            for (const auto& pt : acc["customPoints"]) {
                if (pt.is_object()) {
                    p.acceleration.customPoints.push_back({
                        pt.value("speed", 0.0),
                        pt.value("multiplier", 1.0)
                    });
                }
            }
        }
    }

    return p;
}

Profile Profile::CreateDefault() {
    Profile p;
    p.id = "profile_default";
    p.name = "Default";
    p.enabled = true;
    p.targetApp = "Global";
    p.activationModeStr = "Always active (Base)";
    p.xMultiplier = 1.0;
    p.yMultiplier = 1.0;
    p.overallMultiplier = 1.0;
    p.acceleration.enabled = false;
    p.conditions = {
        { "Base profile", "Always active" }
    };
    return p;
}

Profile Profile::CreatePrecision() {
    Profile p;
    p.id = "profile_precision";
    p.name = "Precision";
    p.enabled = true;
    p.targetApp = "Global";
    p.hotkey.vkCode = VK_SHIFT;
    p.hotkey.modifiers = ModNone;
    p.activationMode = ActivationMode::Hold;
    p.activationModeStr = "While conditions are true";
    p.xMultiplier = 0.50;
    p.yMultiplier = 0.50;
    p.overallMultiplier = 1.0;
    p.acceleration.enabled = false;
    p.conditions = {
        { "Keyboard key held", "Shift held" }
    };
    return p;
}

Profile Profile::CreateSprayControl() {
    Profile p;
    p.id = "profile_spray_control";
    p.name = "Spray Control";
    p.enabled = true;
    p.targetApp = "PUBG";
    p.hotkey.mouseButton = MouseButton::Left;
    p.activationMode = ActivationMode::Hold;
    p.activationModeStr = "While conditions are true";
    p.xMultiplier = 0.65;
    p.yMultiplier = 1.00;
    p.overallMultiplier = 1.00;
    p.acceleration.enabled = true;
    p.acceleration.type = CurveType::Polynomial;
    p.acceleration.strength = 0.80;
    p.acceleration.threshold = 50.0;
    p.acceleration.maxMultiplier = 2.00;
    p.acceleration.exponent = 2.0;
    p.conditions = {
        { "Mouse button held", "RMB held" },
        { "Mouse button held", "LMB held" },
        { "Mouse movement detected", "Mouse movement detected" }
    };
    return p;
}

Profile Profile::CreateFastTurn() {
    Profile p;
    p.id = "profile_fast_turn";
    p.name = "Fast Turn";
    p.enabled = true;
    p.targetApp = "Global";
    p.hotkey.mouseButton = MouseButton::XButton1; // Mouse 4
    p.activationMode = ActivationMode::Hold;
    p.activationModeStr = "While conditions are true";
    p.xMultiplier = 2.0;
    p.yMultiplier = 2.0;
    p.overallMultiplier = 1.0;
    p.acceleration.enabled = true;
    p.acceleration.type = CurveType::Linear;
    p.acceleration.strength = 0.80;
    p.acceleration.maxMultiplier = 2.50;
    p.conditions = {
        { "Mouse button held", "Mouse 4 held" }
    };
    return p;
}

Profile Profile::CreateAiming() {
    Profile p;
    p.id = "profile_aiming";
    p.name = "Aiming";
    p.enabled = true;
    p.hotkey.mouseButton = MouseButton::Right; // RMB
    p.activationMode = ActivationMode::Hold;
    p.activationModeStr = "While conditions are true";
    p.xMultiplier = 0.75;
    p.yMultiplier = 0.75;
    p.overallMultiplier = 1.0;
    p.acceleration.enabled = false;
    p.conditions = {
        { "Mouse button held", "RMB held" }
    };
    return p;
}

Profile Profile::CreateLowSens() {
    Profile p;
    p.id = "profile_low_sens";
    p.name = "Low Sens";
    p.enabled = true;
    p.hotkey.vkCode = 'L';
    p.hotkey.modifiers = ModAlt;
    p.activationMode = ActivationMode::Toggle;
    p.activationModeStr = "Toggle";
    p.xMultiplier = 0.60;
    p.yMultiplier = 0.60;
    p.overallMultiplier = 1.0;
    p.acceleration.enabled = false;
    p.conditions = {
        { "Keyboard key held", "Alt + L toggle" }
    };
    return p;
}

Profile Profile::CreateHighSens() {
    Profile p;
    p.id = "profile_high_sens";
    p.name = "High Sens";
    p.enabled = true;
    p.hotkey.vkCode = 'H';
    p.hotkey.modifiers = ModAlt;
    p.activationMode = ActivationMode::Toggle;
    p.activationModeStr = "Toggle";
    p.xMultiplier = 1.80;
    p.yMultiplier = 1.80;
    p.overallMultiplier = 1.0;
    p.acceleration.enabled = false;
    p.conditions = {
        { "Keyboard key held", "Alt + H toggle" }
    };
    return p;
}
