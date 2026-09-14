#pragma once

#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include <memory>
#include "IAccelerationCurve.h"

class CurveWidget : public QWidget {
    Q_OBJECT
public:
    explicit CurveWidget(QWidget* parent = nullptr);

    void SetCurve(std::shared_ptr<IAccelerationCurve> curve);
    void SetCurrentVelocity(double velocityCountsPerSec);
    void SetMaxDisplayMultiplier(double maxMult);
    void SetMaxDisplayVelocity(double maxVel);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::shared_ptr<IAccelerationCurve> m_curve;
    double m_currentVelocity = 0.0;
    double m_maxDisplayMultiplier = 3.0;
    double m_maxDisplayVelocity = 2000.0;
};
