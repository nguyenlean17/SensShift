#include "Curves.h"

// --- Linear Acceleration ---
LinearAccelerationCurve::LinearAccelerationCurve(const AccelerationSettings& settings)
    : m_settings(settings) {}

double LinearAccelerationCurve::Calculate(double velocity) const {
    double v = std::clamp(velocity, m_settings.minSpeed, m_settings.maxSpeed);
    if (v <= m_settings.threshold) {
        return 1.0 * m_settings.sensMultiplier;
    }
    double excess = v - m_settings.threshold;
    // Normalized strength scaling (at strength=1.0, adds +1.0x per 500 counts/sec)
    double mult = 1.0 + (m_settings.strength * 0.002) * excess;
    mult = std::clamp(mult, 1.0, std::max(1.0, m_settings.maxMultiplier));
    return mult * m_settings.sensMultiplier;
}

// --- Polynomial Acceleration ---
PolynomialAccelerationCurve::PolynomialAccelerationCurve(const AccelerationSettings& settings)
    : m_settings(settings) {}

double PolynomialAccelerationCurve::Calculate(double velocity) const {
    double v = std::clamp(velocity, m_settings.minSpeed, m_settings.maxSpeed);
    if (v <= m_settings.threshold) {
        return 1.0 * m_settings.sensMultiplier;
    }
    double excess = v - m_settings.threshold;
    double normalizedExcess = excess / 1000.0;
    double expVal = std::max(0.1, m_settings.exponent);
    double mult = 1.0 + m_settings.strength * std::pow(normalizedExcess, expVal);
    mult = std::clamp(mult, 1.0, std::max(1.0, m_settings.maxMultiplier));
    return mult * m_settings.sensMultiplier;
}

// --- Exponential Acceleration ---
ExponentialAccelerationCurve::ExponentialAccelerationCurve(const AccelerationSettings& settings)
    : m_settings(settings) {}

double ExponentialAccelerationCurve::Calculate(double velocity) const {
    double v = std::clamp(velocity, m_settings.minSpeed, m_settings.maxSpeed);
    if (v <= m_settings.threshold) {
        return 1.0 * m_settings.sensMultiplier;
    }
    double excess = v - m_settings.threshold;
    double range = std::max(0.0, m_settings.maxMultiplier - 1.0);
    // Smooth asymptotic approach to maxMultiplier
    double mult = 1.0 + range * (1.0 - std::exp(-m_settings.strength * 0.0025 * excess));
    mult = std::clamp(mult, 1.0, std::max(1.0, m_settings.maxMultiplier));
    return mult * m_settings.sensMultiplier;
}

// --- Power Acceleration ---
PowerAccelerationCurve::PowerAccelerationCurve(const AccelerationSettings& settings)
    : m_settings(settings) {}

double PowerAccelerationCurve::Calculate(double velocity) const {
    double v = std::clamp(velocity, m_settings.minSpeed, m_settings.maxSpeed);
    double thresh = std::max(1.0, m_settings.threshold);
    if (v <= thresh) {
        return 1.0 * m_settings.sensMultiplier;
    }
    double ratio = v / thresh;
    double expVal = std::max(0.0, m_settings.exponent - 1.0);
    double mult = 1.0 + m_settings.strength * (std::pow(ratio, expVal) - 1.0);
    mult = std::clamp(mult, 1.0, std::max(1.0, m_settings.maxMultiplier));
    return mult * m_settings.sensMultiplier;
}

// --- Custom LUT Acceleration ---
CustomLutAccelerationCurve::CustomLutAccelerationCurve(const AccelerationSettings& settings)
    : m_settings(settings), m_sortedPoints(settings.customPoints) {
    std::sort(m_sortedPoints.begin(), m_sortedPoints.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });
}

double CustomLutAccelerationCurve::Calculate(double velocity) const {
    if (m_sortedPoints.empty()) {
        return 1.0 * m_settings.sensMultiplier;
    }
    double v = std::clamp(velocity, m_settings.minSpeed, m_settings.maxSpeed);
    if (v <= m_sortedPoints.front().first) {
        return m_sortedPoints.front().second * m_settings.sensMultiplier;
    }
    if (v >= m_sortedPoints.back().first) {
        return m_sortedPoints.back().second * m_settings.sensMultiplier;
    }

    // Binary search for interval
    for (size_t i = 0; i < m_sortedPoints.size() - 1; ++i) {
        if (v >= m_sortedPoints[i].first && v <= m_sortedPoints[i + 1].first) {
            double v0 = m_sortedPoints[i].first;
            double v1 = m_sortedPoints[i + 1].first;
            double m0 = m_sortedPoints[i].second;
            double m1 = m_sortedPoints[i + 1].second;
            double t = (v1 > v0) ? ((v - v0) / (v1 - v0)) : 0.0;
            double mult = m0 + t * (m1 - m0);
            return mult * m_settings.sensMultiplier;
        }
    }
    return m_sortedPoints.back().second * m_settings.sensMultiplier;
}

// --- Factory ---
std::unique_ptr<IAccelerationCurve> CreateAccelerationCurve(const AccelerationSettings& settings) {
    switch (settings.type) {
        case CurveType::Polynomial:
            return std::make_unique<PolynomialAccelerationCurve>(settings);
        case CurveType::Exponential:
            return std::make_unique<ExponentialAccelerationCurve>(settings);
        case CurveType::Power:
            return std::make_unique<PowerAccelerationCurve>(settings);
        case CurveType::CustomLUT:
            return std::make_unique<CustomLutAccelerationCurve>(settings);
        case CurveType::Linear:
        default:
            return std::make_unique<LinearAccelerationCurve>(settings);
    }
}
