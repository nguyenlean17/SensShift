#pragma once

#include <cmath>
#include <cstdint>

class FractionalAccumulator {
public:
    FractionalAccumulator() = default;

    // Accumulate fractional deltas, emit whole integer counts, and retain sub-pixel remainders.
    // Handles positive and negative numbers symmetrically without loss or bias.
    void Process(double inX, double inY, int& outX, int& outY) {
        double totalX = inX + m_remainderX;
        double totalY = inY + m_remainderY;

        int intX = static_cast<int>(totalX);
        int intY = static_cast<int>(totalY);

        m_remainderX = totalX - static_cast<double>(intX);
        m_remainderY = totalY - static_cast<double>(intY);

        outX = intX;
        outY = intY;
    }

    void Reset() {
        m_remainderX = 0.0;
        m_remainderY = 0.0;
    }

    double GetRemainderX() const { return m_remainderX; }
    double GetRemainderY() const { return m_remainderY; }

private:
    double m_remainderX = 0.0;
    double m_remainderY = 0.0;
};
