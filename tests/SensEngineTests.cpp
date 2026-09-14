#include "FractionalAccumulator.h"
#include "Curves.h"
#include "InputDefines.h"
#include "Profile.h"
#include "TransformationPipeline.h"
#include <iostream>
#include <cassert>
#include <cmath>

#define TEST_ASSERT(expr, msg) \
    if (!(expr)) { \
        std::cerr << "FAIL: " << msg << " (" << #expr << ") at line " << __LINE__ << std::endl; \
        return 1; \
    } else { \
        std::cout << "PASS: " << msg << std::endl; \
    }

int main() {
    std::cout << "==========================================\n";
    std::cout << " Running MouseSensitivityModifier Tests   \n";
    std::cout << "==========================================\n";

    // 1. FractionalAccumulator Tests
    {
        FractionalAccumulator acc;

        // Test 1: Positive fractional accumulation (4 steps of 0.25 should equal 1)
        int outX = 0, outY = 0;
        acc.Process(0.25, 0.25, outX, outY);
        TEST_ASSERT(outX == 0 && outY == 0, "Step 1 (0.25) outputs 0");
        acc.Process(0.25, 0.25, outX, outY);
        TEST_ASSERT(outX == 0 && outY == 0, "Step 2 (0.50) outputs 0");
        acc.Process(0.25, 0.25, outX, outY);
        TEST_ASSERT(outX == 0 && outY == 0, "Step 3 (0.75) outputs 0");
        acc.Process(0.25, 0.25, outX, outY);
        TEST_ASSERT(outX == 1 && outY == 1, "Step 4 (1.00) outputs 1");

        // Test 2: 10,000 steps of 0.3333333333333333
        acc.Reset();
        int totalOutX = 0;
        int totalOutY = 0;
        double step = 1.0 / 3.0;
        for (int i = 0; i < 9999; ++i) {
            acc.Process(step, step, outX, outY);
            totalOutX += outX;
            totalOutY += outY;
        }
        TEST_ASSERT(totalOutX == 3333 && totalOutY == 3333, "9999 steps of 1/3 accurately accumulate to 3333");

        // Test 3: Negative numbers symmetry
        acc.Reset();
        int negOutX = 0, negOutY = 0;
        for (int i = 0; i < 4; ++i) {
            acc.Process(-0.75, -0.75, outX, outY);
            negOutX += outX;
            negOutY += outY;
        }
        TEST_ASSERT(negOutX == -3 && negOutY == -3, "4 steps of -0.75 accumulate exactly to -3");
    }

    // 2. Acceleration Curves Tests
    {
        AccelerationSettings settings;
        settings.enabled = true;
        settings.threshold = 100.0;
        settings.maxMultiplier = 2.5;
        settings.strength = 1.0;

        // Linear Curve
        LinearAccelerationCurve lin(settings);
        TEST_ASSERT(lin.Calculate(50.0) == 1.0, "Linear: Below threshold returns 1.0x");
        double midLinear = lin.Calculate(350.0); // excess = 250 -> 1.0 + 1.0 * 0.002 * 250 = 1.50
        TEST_ASSERT(std::abs(midLinear - 1.50) < 0.01, "Linear: Calculates expected 1.50x multiplier");
        double cappedLinear = lin.Calculate(5000.0);
        TEST_ASSERT(cappedLinear == 2.5, "Linear: Clamps to maxMultiplier (2.5x)");

        // Polynomial Curve
        PolynomialAccelerationCurve poly(settings);
        TEST_ASSERT(poly.Calculate(50.0) == 1.0, "Polynomial: Below threshold returns 1.0x");
        TEST_ASSERT(poly.Calculate(5000.0) == 2.5, "Polynomial: Clamps to maxMultiplier (2.5x)");

        // Exponential Curve
        ExponentialAccelerationCurve expCurve(settings);
        TEST_ASSERT(expCurve.Calculate(50.0) == 1.0, "Exponential: Below threshold returns 1.0x");
        double highExp = expCurve.Calculate(2000.0);
        TEST_ASSERT(highExp > 1.0 && highExp <= 2.5, "Exponential: Stays bounded within (1.0x, 2.5x]");

        // Custom LUT Curve
        CustomLutAccelerationCurve lut(settings);
        double atZero = lut.Calculate(0.0);
        TEST_ASSERT(std::abs(atZero - 1.0) < 0.01, "LUT: At 0 counts/sec multiplier is 1.00");
        double atInterp = lut.Calculate(300.0); // between 100 (1.05) and 500 (1.25): 1.05 + 0.5 * 0.20 = 1.15
        TEST_ASSERT(std::abs(atInterp - 1.15) < 0.02, "LUT: Interpolates between defined points");
    }

    // 3. Hotkey Formatting Tests
    {
        Hotkey hk1;
        hk1.vkCode = 'X';
        hk1.modifiers = ModCtrl | ModAlt;
        TEST_ASSERT(hk1.ToString() == "Ctrl + Alt + X", "Hotkey formats Ctrl + Alt + X correctly");

        Hotkey hk2;
        hk2.mouseButton = MouseButton::XButton1;
        hk2.modifiers = ModShift;
        TEST_ASSERT(hk2.ToString() == "Shift + Mouse 4", "Hotkey formats Shift + Mouse 4 correctly");

        Hotkey hk3;
        hk3.mouseButton = MouseButton::Left;
        hk3.modifiers = ModNone;
        TEST_ASSERT(hk3.ToString() == "LMB", "Hotkey formats LMB correctly");
    }

    // 4. Profile JSON Roundtrip
    {
        Profile pOrig = Profile::CreateFastTurn();
        nlohmann::json j = pOrig.ToJson();
        Profile pLoaded = Profile::FromJson(j);

        TEST_ASSERT(pLoaded.name == pOrig.name, "JSON Roundtrip: Name matches");
        TEST_ASSERT(pLoaded.xMultiplier == pOrig.xMultiplier, "JSON Roundtrip: X Multiplier matches");
        TEST_ASSERT(pLoaded.yMultiplier == pOrig.yMultiplier, "JSON Roundtrip: Y Multiplier matches");
        TEST_ASSERT(pLoaded.hotkey.mouseButton == MouseButton::XButton1, "JSON Roundtrip: Mouse 4 hotkey matches");
        TEST_ASSERT(pLoaded.acceleration.enabled == true, "JSON Roundtrip: Acceleration enabled matches");
        TEST_ASSERT(pLoaded.acceleration.maxMultiplier == 2.5, "JSON Roundtrip: Max multiplier matches");
    }

    // 5. TransformationPipeline Tests
    {
        TransformationPipeline pipeline;
        AccelerationSettings noAccel;
        noAccel.enabled = false;

        // Configure 0.5x sensitivity
        pipeline.Configure("Test", 0.5, 0.5, 1.0, noAccel, TransformationOrder::SensThenAccel, true, false);

        int outX = 0, outY = 0;
        pipeline.Transform(10, 20, outX, outY);
        TEST_ASSERT(outX == 5 && outY == 10, "Pipeline: Scales (10, 20) by 0.5x to (5, 10)");

        // Failsafe disable
        pipeline.Configure("Test", 0.5, 0.5, 1.0, noAccel, TransformationOrder::SensThenAccel, true, true);
        pipeline.Transform(10, 20, outX, outY);
        TEST_ASSERT(outX == 10 && outY == 20, "Pipeline: Failsafe disables scaling and passes through (10, 20)");
    }

    std::cout << "\nALL UNIT AND ENGINE TESTS PASSED SUCCESSFULLY!\n";
    return 0;
}
