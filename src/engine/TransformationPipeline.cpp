#include "TransformationPipeline.h"
#include <cmath>
#include <algorithm>

TransformationPipeline::TransformationPipeline() {
    QueryPerformanceFrequency(&m_perfFreq);
    QueryPerformanceCounter(&m_lastTime);
}

void TransformationPipeline::Configure(const std::string& profileName,
                                       double xMult,
                                       double yMult,
                                       double overallMult,
                                       const AccelerationSettings& accelSettings,
                                       TransformationOrder order,
                                       bool modifierActive,
                                       bool emergencyDisabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_profileName = profileName;
    m_xMultiplier = xMult;
    m_yMultiplier = yMult;
    m_overallMultiplier = overallMult;
    m_accelerationEnabled = accelSettings.enabled;
    m_curve = CreateAccelerationCurve(accelSettings);
    m_order = order;
    m_modifierActive = modifierActive;
    m_emergencyDisabled = emergencyDisabled;

    if (!modifierActive || emergencyDisabled) {
        m_accumulator.Reset();
    }
}

void TransformationPipeline::Transform(int rawX, int rawY, int& outX, int& outY) {
    PipelineTelemetry newTel; // Build telemetry locally, commit under separate lock

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // If failsafe disabled or no modifier active, pass through physical delta directly
        if (m_emergencyDisabled || !m_modifierActive) {
            outX = rawX;
            outY = rawY;

            newTel.rawX = rawX;
            newTel.rawY = rawY;
            newTel.velocity = 0.0;
            newTel.accelMultiplier = 1.0;
            newTel.effXMultiplier = 1.0;
            newTel.effYMultiplier = 1.0;
            newTel.outX = outX;
            newTel.outY = outY;
            newTel.profileName = m_profileName;
            newTel.isModifierActive = m_modifierActive;
            newTel.isEmergencyDisabled = m_emergencyDisabled;

            // Commit telemetry under separate lock (after releasing m_mutex)
        } else {

    // High resolution time delta calculation
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    double dt = 0.002; // Default fallback: 2ms (~500Hz)
    if (m_hasLastTime && m_perfFreq.QuadPart > 0) {
        dt = static_cast<double>(now.QuadPart - m_lastTime.QuadPart) / static_cast<double>(m_perfFreq.QuadPart);
        // Clamp bounds: 0.25ms (4000Hz) to 100ms (10Hz)
        dt = std::clamp(dt, 0.00025, 0.100);
    }
    m_lastTime = now;
    m_hasLastTime = true;

    double effXSens = m_xMultiplier * m_overallMultiplier;
    double effYSens = m_yMultiplier * m_overallMultiplier;
    double accelMult = 1.0;
    double velocity = 0.0;
    double targetX = 0.0;
    double targetY = 0.0;

    if (m_order == TransformationOrder::SensThenAccel) {
        // Step 1: Scale by sensitivity
        double sensX = static_cast<double>(rawX) * effXSens;
        double sensY = static_cast<double>(rawY) * effYSens;

        // Step 2: Compute velocity from scaled movement
        double dist = std::sqrt(sensX * sensX + sensY * sensY);
        velocity = dist / dt;

        // Step 3: Compute acceleration multiplier
        if (m_accelerationEnabled && m_curve) {
            accelMult = m_curve->Calculate(velocity);
        }

        // Step 4: Apply acceleration
        targetX = sensX * accelMult;
        targetY = sensY * accelMult;
    } else {
        // AccelThenSens:
        // Step 1: Compute velocity from raw movement
        double rawDist = std::sqrt(static_cast<double>(rawX * rawX + rawY * rawY));
        velocity = rawDist / dt;

        // Step 2: Compute acceleration multiplier
        if (m_accelerationEnabled && m_curve) {
            accelMult = m_curve->Calculate(velocity);
        }

        // Step 3: Apply acceleration, then sensitivity
        targetX = static_cast<double>(rawX) * accelMult * effXSens;
        targetY = static_cast<double>(rawY) * accelMult * effYSens;
    }

        // Step 5: Fractional accumulation to prevent integer rounding loss
        m_accumulator.Process(targetX, targetY, outX, outY);

        // Build telemetry
        newTel.rawX = rawX;
        newTel.rawY = rawY;
        newTel.velocity = velocity;
        newTel.accelMultiplier = accelMult;
        newTel.effXMultiplier = effXSens * accelMult;
        newTel.effYMultiplier = effYSens * accelMult;
        newTel.outX = outX;
        newTel.outY = outY;
        newTel.profileName = m_profileName;
        newTel.isModifierActive = m_modifierActive;
        newTel.isEmergencyDisabled = m_emergencyDisabled;
        }
    } // m_mutex released here

    // Commit telemetry under separate lock — never nests with m_mutex.
    // Use try_to_lock so the hook thread NEVER stalls waiting for the UI thread.
    {
        std::unique_lock<std::mutex> telLock(m_telemetryMutex, std::try_to_lock);
        if (telLock.owns_lock()) {
            m_telemetry = std::move(newTel);
        }
    }
}

void TransformationPipeline::Reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_accumulator.Reset();
    m_hasLastTime = false;
}

PipelineTelemetry TransformationPipeline::GetTelemetry() const {
    std::lock_guard<std::mutex> lock(m_telemetryMutex);
    return m_telemetry;
}
