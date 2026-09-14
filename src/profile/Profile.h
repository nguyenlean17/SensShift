#pragma once

#include "InputDefines.h"
#include "IAccelerationCurve.h"
#include "json.hpp"
#include <string>
#include <vector>

struct ActivationCondition {
    std::string type;
    std::string text;
};

struct Profile {
    std::string id;
    std::string name = "New Profile";
    bool enabled = true;
    std::string targetApp = "PUBG";

    Hotkey hotkey;
    ActivationMode activationMode = ActivationMode::Hold;
    std::string activationModeStr = "While conditions are true";
    bool blockOriginalHotkeyInput = false;

    std::vector<ActivationCondition> conditions;

    double xMultiplier = 1.0;
    double yMultiplier = 1.0;
    double overallMultiplier = 1.0;

    AccelerationSettings acceleration;
    TransformationOrder order = TransformationOrder::SensThenAccel;

    nlohmann::json ToJson() const;
    static Profile FromJson(const nlohmann::json& j);

    static Profile CreateDefault();
    static Profile CreatePrecision();
    static Profile CreateSprayControl();
    static Profile CreateFastTurn();
    static Profile CreateAiming();
    static Profile CreateLowSens();
    static Profile CreateHighSens();
};
