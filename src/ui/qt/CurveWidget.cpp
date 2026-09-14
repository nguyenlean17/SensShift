#include "CurveWidget.h"
#include <algorithm>
#include <cmath>

CurveWidget::CurveWidget(QWidget* parent)
    : QWidget(parent) {
    setMinimumSize(400, 240);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void CurveWidget::SetCurve(std::shared_ptr<IAccelerationCurve> curve) {
    m_curve = std::move(curve);
    update();
}

void CurveWidget::SetCurrentVelocity(double velocityCountsPerSec) {
    m_currentVelocity = velocityCountsPerSec;
    update();
}

void CurveWidget::SetMaxDisplayMultiplier(double maxMult) {
    m_maxDisplayMultiplier = std::max(1.5, maxMult);
    update();
}

void CurveWidget::SetMaxDisplayVelocity(double maxVel) {
    m_maxDisplayVelocity = std::max(500.0, maxVel);
    update();
}

void CurveWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();
    const int marginL = 50;
    const int marginR = 20;
    const int marginT = 20;
    const int marginB = 35;

    const int plotW = w - marginL - marginR;
    const int plotH = h - marginT - marginB;

    if (plotW <= 10 || plotH <= 10) return;

    // Background
    painter.fillRect(rect(), QColor(24, 24, 24));
    painter.fillRect(QRect(marginL, marginT, plotW, plotH), QColor(18, 18, 18));

    // Grid lines
    painter.setPen(QPen(QColor(40, 40, 40), 1, Qt::DashLine));
    for (int i = 1; i <= 4; ++i) {
        int y = marginT + plotH - (plotH * i / 4);
        painter.drawLine(marginL, y, marginL + plotW, y);

        double multVal = (m_maxDisplayMultiplier * i) / 4.0;
        painter.setPen(QColor(120, 120, 120));
        painter.drawText(QRect(0, y - 10, marginL - 8, 20), Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(multVal, 'f', 1) + "x");
        painter.setPen(QPen(QColor(40, 40, 40), 1, Qt::DashLine));
    }

    for (int i = 1; i <= 4; ++i) {
        int x = marginL + (plotW * i / 4);
        painter.drawLine(x, marginT, x, marginT + plotH);

        double velVal = (m_maxDisplayVelocity * i) / 4.0;
        painter.setPen(QColor(120, 120, 120));
        painter.drawText(QRect(x - 40, marginT + plotH + 6, 80, 20), Qt::AlignCenter,
                         QString::number(static_cast<int>(velVal)));
        painter.setPen(QPen(QColor(40, 40, 40), 1, Qt::DashLine));
    }

    // Axis labels
    painter.setPen(QColor(150, 150, 150));
    painter.drawText(QRect(marginL, marginT + plotH + 18, plotW, 16), Qt::AlignCenter, "Velocity (counts/sec)");

    // Curve plotting
    if (m_curve && plotW > 0 && plotH > 0) {
        QPainterPath path;
        QPainterPath fillPath;

        const int steps = std::clamp(plotW, 50, 400);
        bool first = true;

        for (int i = 0; i <= steps; ++i) {
            double normX = static_cast<double>(i) / steps;
            double velocity = normX * m_maxDisplayVelocity;
            double mult = m_curve->Calculate(velocity);

            double normY = std::clamp(mult / m_maxDisplayMultiplier, 0.0, 1.0);

            double ptX = marginL + normX * plotW;
            double ptY = marginT + plotH - (normY * plotH);

            if (first) {
                path.moveTo(ptX, ptY);
                fillPath.moveTo(ptX, marginT + plotH);
                fillPath.lineTo(ptX, ptY);
                first = false;
            } else {
                path.lineTo(ptX, ptY);
                fillPath.lineTo(ptX, ptY);
            }
        }
        fillPath.lineTo(marginL + plotW, marginT + plotH);
        fillPath.closeSubpath();

        // Gradient fill below curve
        QLinearGradient grad(marginL, marginT, marginL, marginT + plotH);
        grad.setColorAt(0.0, QColor(226, 35, 26, 80));
        grad.setColorAt(1.0, QColor(226, 35, 26, 0));
        painter.fillPath(fillPath, grad);

        // Curve stroke
        painter.setPen(QPen(QColor(226, 35, 26), 2.5));
        painter.drawPath(path);

        // Current velocity dot marker
        if (m_currentVelocity > 0.0) {
            double dotNormX = std::clamp(m_currentVelocity / m_maxDisplayVelocity, 0.0, 1.0);
            double dotMult = m_curve->Calculate(m_currentVelocity);
            double dotNormY = std::clamp(dotMult / m_maxDisplayMultiplier, 0.0, 1.0);

            double dotX = marginL + dotNormX * plotW;
            double dotY = marginT + plotH - (dotNormY * plotH);

            // Glow circle
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(255, 60, 50, 100));
            painter.drawEllipse(QPointF(dotX, dotY), 8, 8);

            // Center solid dot
            painter.setBrush(QColor(255, 255, 255));
            painter.drawEllipse(QPointF(dotX, dotY), 4, 4);

            // Tooltip text near dot
            painter.setPen(QColor(255, 255, 255));
            QString info = QString("%1x @ %2 c/s")
                           .arg(dotMult, 0, 'f', 2)
                           .arg(static_cast<int>(m_currentVelocity));
            painter.drawText(QPointF(dotX + 8, dotY - 6), info);
        }
    }

    // Border around plot
    painter.setPen(QPen(QColor(55, 55, 55), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(marginL, marginT, plotW, plotH);
}
