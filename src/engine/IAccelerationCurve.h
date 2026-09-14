#pragma once

#include "InputDefines.h"
#include <string>
#include <vector>
#include <memory>

struct AccelerationSettings {
    bool enabled = false;
    CurveType type = CurveType::Linear;
    double strength = 0.5;          // Scale factor for acceleration
    double threshold = 50.0;        // Velocity threshold in counts/sec before accel activates
    double maxMultiplier = 2.5;     // Cap on total acceleration multiplier
    double exponent = 2.0;          // Exponent for polynomial or power curves
    double minSpeed = 0.0;          // Minimum velocity clamp
    double maxSpeed = 3000.0;       // Maximum velocity clamp
    double sensMultiplier = 1.0;    // Additional sensitivity coefficient

    // Custom points (velocity -> multiplier) for CustomLUT curve
    std::vector<std::pair<double, double>> customPoints = {
        {0.0, 1.0},
        {100.0, 1.05},
        {500.0, 1.25},
        {1000.0, 1.60},
        {2000.0, 2.00}
    };
};

class IAccelerationCurve {
public:
    virtual ~IAccelerationCurve() = default;

    // Calculate acceleration multiplier given instantaneous velocity (counts per second)
    virtual double Calculate(double velocity) const = 0;

    virtual CurveType GetType() const = 0;
    virtual std::string GetName() const = 0;
};
