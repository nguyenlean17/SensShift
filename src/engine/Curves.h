#pragma once

#include "IAccelerationCurve.h"
#include <algorithm>
#include <cmath>

class LinearAccelerationCurve : public IAccelerationCurve {
public:
    explicit LinearAccelerationCurve(const AccelerationSettings& settings);
    double Calculate(double velocity) const override;
    CurveType GetType() const override { return CurveType::Linear; }
    std::string GetName() const override { return "Linear"; }

private:
    AccelerationSettings m_settings;
};

class PolynomialAccelerationCurve : public IAccelerationCurve {
public:
    explicit PolynomialAccelerationCurve(const AccelerationSettings& settings);
    double Calculate(double velocity) const override;
    CurveType GetType() const override { return CurveType::Polynomial; }
    std::string GetName() const override { return "Polynomial"; }

private:
    AccelerationSettings m_settings;
};

class ExponentialAccelerationCurve : public IAccelerationCurve {
public:
    explicit ExponentialAccelerationCurve(const AccelerationSettings& settings);
    double Calculate(double velocity) const override;
    CurveType GetType() const override { return CurveType::Exponential; }
    std::string GetName() const override { return "Exponential"; }

private:
    AccelerationSettings m_settings;
};

class PowerAccelerationCurve : public IAccelerationCurve {
public:
    explicit PowerAccelerationCurve(const AccelerationSettings& settings);
    double Calculate(double velocity) const override;
    CurveType GetType() const override { return CurveType::Power; }
    std::string GetName() const override { return "Power"; }

private:
    AccelerationSettings m_settings;
};

class CustomLutAccelerationCurve : public IAccelerationCurve {
public:
    explicit CustomLutAccelerationCurve(const AccelerationSettings& settings);
    double Calculate(double velocity) const override;
    CurveType GetType() const override { return CurveType::CustomLUT; }
    std::string GetName() const override { return "Custom LUT"; }

private:
    AccelerationSettings m_settings;
    std::vector<std::pair<double, double>> m_sortedPoints;
};

// Factory function to instantiate curve based on settings
std::unique_ptr<IAccelerationCurve> CreateAccelerationCurve(const AccelerationSettings& settings);
