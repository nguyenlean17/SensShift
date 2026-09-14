#pragma once

#include "InputDefines.h"
#include "IAccelerationCurve.h"
#include "Curves.h"
#include "FractionalAccumulator.h"
#include <memory>
#include <mutex>
#include <string>
#include <atomic>
#include <chrono>

struct PipelineTelemetry {
    int rawX = 0;
    int rawY = 0;
    double velocity = 0.0;
    double accelMultiplier = 1.0;
    double effXMultiplier = 1.0;
    double effYMultiplier = 1.0;
    int outX = 0;
    int outY = 0;
    std::string profileName = "Default";
    bool isModifierActive = false;
    bool isEmergencyDisabled = false;
};

class TransformationPipeline {
public:
    TransformationPipeline();
    ~TransformationPipeline() = default;

    // Set transformation parameters from profile
    void Configure(const std::string& profileName,
                   double xMult,
                   double yMult,
                   double overallMult,
                   const AccelerationSettings& accelSettings,
                   TransformationOrder order,
                   bool modifierActive,
                   bool emergencyDisabled);

    // Process a raw mouse delta and produce modified output delta
    void Transform(int rawX, int rawY, int& outX, int& outY);

    // Reset accumulator state
    void Reset();

    // Query latest telemetry snapshot for UI / debug HUD
    PipelineTelemetry GetTelemetry() const;

private:
    // Main mutex: protects config + transform state. Held by hook threads during Transform()
    // and briefly during Configure(). NEVER acquire m_telemetryMutex while holding this.
    mutable std::mutex m_mutex;

    // Telemetry mutex: protects ONLY the telemetry snapshot. Acquired independently
    // by the UI thread via GetTelemetry(). Breaking the ABBA cycle.
    mutable std::mutex m_telemetryMutex;

    std::string m_profileName = "Default";
    double m_xMultiplier = 1.0;
    double m_yMultiplier = 1.0;
    double m_overallMultiplier = 1.0;
    bool m_accelerationEnabled = false;
    std::unique_ptr<IAccelerationCurve> m_curve;
    TransformationOrder m_order = TransformationOrder::SensThenAccel;
    bool m_modifierActive = false;
    bool m_emergencyDisabled = false;

    FractionalAccumulator m_accumulator;

    // High resolution timing
    LARGE_INTEGER m_perfFreq{};
    LARGE_INTEGER m_lastTime{};
    bool m_hasLastTime = false;

    // Telemetry cache (protected by m_telemetryMutex, NOT m_mutex)
    PipelineTelemetry m_telemetry;
};
