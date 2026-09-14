#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QGroupBox>
#include <QMessageBox>
#include <QApplication>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <algorithm>

MainWindow* MainWindow::s_instance = nullptr;

MainWindow::MainWindow(ProfileManager& profileManager,
                       ConfigManager& configManager,
                       TransformationPipeline& pipeline,
                       MouseInterceptor& mouseInterceptor,
                       KeyboardHotkeyManager& keyboardManager,
                       QWidget* parent)
    : QMainWindow(parent),
      m_profileManager(profileManager),
      m_configManager(configManager),
      m_pipeline(pipeline),
      m_mouseInterceptor(mouseInterceptor),
      m_keyboardManager(keyboardManager) {

    s_instance = this;

    setWindowTitle("Windows Mouse Sensitivity & Acceleration Modifier");
    resize(1080, 760);
    setMinimumSize(980, 680);

    ApplyDarkStyle();
    SetupUi();
    RefreshProfileList();

    // Hook up keyboard and mouse recorder callbacks for hotkey recording
    auto inputRecorder = [this](uint32_t vkCode, MouseButton btn, uint32_t mods) -> bool {
        if (m_isRecordingHotkey) {
            // If it's a modifier key by itself, ignore until the actual key/button is pressed
            if (btn == MouseButton::None && (vkCode == VK_SHIFT || vkCode == VK_LSHIFT || vkCode == VK_RSHIFT ||
                                             vkCode == VK_CONTROL || vkCode == VK_LCONTROL || vkCode == VK_RCONTROL ||
                                             vkCode == VK_MENU || vkCode == VK_LMENU || vkCode == VK_RMENU)) {
                return true;
            }
            QMetaObject::invokeMethod(this, [this, vkCode, btn, mods]() {
                OnGlobalInputRecorded(vkCode, btn, mods);
            }, Qt::QueuedConnection);
            return true; // Consume event while recording
        }
        return false;
    };

    m_keyboardManager.SetRecorderCallback(inputRecorder);
    m_mouseInterceptor.SetRecorderCallback(inputRecorder);

    m_hudTimer = new QTimer(this);
    connect(m_hudTimer, &QTimer::timeout, this, &MainWindow::OnHudTimerTimeout);
    m_hudTimer->start(100); // ~10 FPS HUD update — fast enough for telemetry, avoids event loop congestion
}

MainWindow::~MainWindow() {
    // Clear hook callbacks BEFORE this object is destroyed to prevent
    // dangling pointer dereference from hook threads
    m_keyboardManager.ClearRecorderCallback();
    m_mouseInterceptor.ClearRecorderCallback();

    if (s_instance == this) {
        s_instance = nullptr;
    }
}

void MainWindow::ApplyDarkStyle() {
    setStyleSheet(R"(
        QMainWindow {
            background-color: #16181E;
        }
        QWidget {
            font-family: 'Segoe UI', sans-serif;
            color: #E1E6F0;
            font-size: 13px;
        }
        QScrollArea, QScrollArea > QWidget > QWidget {
            background-color: transparent;
            background: transparent;
            border: none;
        }
        QGroupBox {
            font-size: 14px;
            font-weight: bold;
            color: #FFFFFF;
            border: 1px solid #2B303C;
            border-radius: 6px;
            margin-top: 14px;
            padding-top: 18px;
            background-color: #1C1E26;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 10px;
            top: 2px;
            padding: 2px 6px;
            color: #FFFFFF;
            background-color: #16181E;
            border-radius: 3px;
        }
        QListWidget {
            background-color: #1C1E26;
            border: 1px solid #2B303C;
            border-radius: 6px;
            padding: 4px;
            color: #E1E6F0;
            outline: none;
        }
        QListWidget::item {
            padding: 8px 10px;
            border-radius: 4px;
            margin-bottom: 2px;
        }
        QListWidget::item:hover {
            background-color: #262B36;
        }
        QListWidget::item:selected {
            background-color: #E2231A;
            color: #FFFFFF;
            font-weight: bold;
        }
        QLineEdit, QTextEdit {
            background-color: #12141A;
            border: 1px solid #2B303C;
            border-radius: 4px;
            padding: 5px 8px;
            color: #FFFFFF;
            font-size: 13px;
        }
        QLineEdit:focus, QTextEdit:focus {
            border-color: #E2231A;
        }
        QTextEdit {
            font-family: 'Consolas', 'Courier New', monospace;
            font-size: 12px;
            line-height: 1.4;
        }
        QComboBox {
            background-color: #12141A;
            border: 1px solid #2B303C;
            border-radius: 4px;
            padding: 5px 10px;
            color: #FFFFFF;
        }
        QComboBox:hover {
            border-color: #4A5263;
        }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        QComboBox QAbstractItemView {
            background-color: #16181E;
            border: 1px solid #2B303C;
            color: #FFFFFF;
            selection-background-color: #E2231A;
            selection-color: #FFFFFF;
            padding: 4px;
        }
        QDoubleSpinBox, QSpinBox {
            background-color: #12141A;
            border: 1px solid #2B303C;
            border-radius: 4px;
            padding: 4px 6px;
            color: #FFFFFF;
            font-weight: bold;
        }
        QDoubleSpinBox:focus, QSpinBox:focus {
            border-color: #E2231A;
        }
        QSlider::groove:horizontal {
            border: 1px solid #2B303C;
            height: 6px;
            background: #12141A;
            border-radius: 3px;
        }
        QSlider::sub-page:horizontal {
            background: #E2231A;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            background: #FFFFFF;
            border: 2px solid #E2231A;
            width: 14px;
            margin: -5px 0;
            border-radius: 7px;
        }
        QSlider::handle:horizontal:hover {
            background: #FFDDDD;
        }
        QPushButton {
            background-color: #222630;
            border: 1px solid #2B303C;
            border-radius: 4px;
            padding: 6px 12px;
            color: #E1E6F0;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: #2B303C;
            border-color: #4A5263;
            color: #FFFFFF;
        }
        QPushButton:pressed {
            background-color: #16181E;
        }
        QCheckBox {
            spacing: 8px;
            color: #E1E6F0;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border-radius: 3px;
            border: 1px solid #2B303C;
            background-color: #12141A;
        }
        QCheckBox::indicator:checked {
            background-color: #E2231A;
            border-color: #FF3333;
        }
        QScrollBar:vertical {
            border: none;
            background: #12141A;
            width: 8px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: #2B303C;
            min-height: 20px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical:hover {
            background: #3D4454;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
    )");
}

void MainWindow::SetupUi() {
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(16);

    // ==========================================
    // 1. LEFT PANEL: Profiles
    // ==========================================
    auto* leftContainer = new QWidget(centralWidget);
    leftContainer->setFixedWidth(220);
    auto* leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(10);

    auto* lblProfilesHeader = new QLabel("Profiles", leftContainer);
    lblProfilesHeader->setStyleSheet("font-size: 16px; font-weight: bold; color: #FFFFFF;");
    leftLayout->addWidget(lblProfilesHeader);

    m_profileList = new QListWidget(leftContainer);
    connect(m_profileList, &QListWidget::currentRowChanged, this, &MainWindow::OnProfileSelectionChanged);
    leftLayout->addWidget(m_profileList, 1);

    auto* btnGrid = new QGridLayout();
    btnGrid->setSpacing(8);

    m_btnNew = new QPushButton("+ New", leftContainer);
    connect(m_btnNew, &QPushButton::clicked, this, &MainWindow::OnNewProfileClicked);
    btnGrid->addWidget(m_btnNew, 0, 0);

    m_btnDup = new QPushButton("Duplicate", leftContainer);
    connect(m_btnDup, &QPushButton::clicked, this, &MainWindow::OnDuplicateProfileClicked);
    btnGrid->addWidget(m_btnDup, 0, 1);

    m_btnDel = new QPushButton("Delete", leftContainer);
    connect(m_btnDel, &QPushButton::clicked, this, &MainWindow::OnDeleteProfileClicked);
    btnGrid->addWidget(m_btnDel, 1, 0);

    m_btnSave = new QPushButton("Save Config", leftContainer);
    m_btnSave->setStyleSheet("background-color: #E2231A; color: white; font-weight: bold; border: none;");
    connect(m_btnSave, &QPushButton::clicked, this, &MainWindow::OnSaveConfigClicked);
    btnGrid->addWidget(m_btnSave, 1, 1);

    leftLayout->addLayout(btnGrid);
    mainLayout->addWidget(leftContainer);

    // ==========================================
    // 2. MIDDLE PANEL: Profile Configuration
    // ==========================================
    auto* midScroll = new QScrollArea(centralWidget);
    midScroll->setWidgetResizable(true);
    midScroll->setFrameShape(QFrame::NoFrame);

    auto* midContainer = new QWidget(midScroll);
    auto* midLayout = new QVBoxLayout(midContainer);
    midLayout->setContentsMargins(0, 0, 10, 0);
    midLayout->setSpacing(12);

    auto* lblConfigHeader = new QLabel("Profile Configuration", midContainer);
    lblConfigHeader->setStyleSheet("font-size: 16px; font-weight: bold; color: #FFFFFF;");
    midLayout->addWidget(lblConfigHeader);

    // Profile Identity & Trigger Group
    auto* grpGeneral = new QGroupBox("Activation & Trigger", midContainer);
    auto* genLayout = new QGridLayout(grpGeneral);
    genLayout->setContentsMargins(12, 16, 12, 14);
    genLayout->setHorizontalSpacing(10);
    genLayout->setVerticalSpacing(10);

    genLayout->addWidget(new QLabel("Profile Name:"), 0, 0);
    m_editName = new QLineEdit(grpGeneral);
    connect(m_editName, &QLineEdit::textChanged, this, &MainWindow::OnProfileNameChanged);
    genLayout->addWidget(m_editName, 0, 1, 1, 2);

    genLayout->addWidget(new QLabel("Hotkey:"), 1, 0);
    m_btnHotkey = new QPushButton("Bind Hotkey...", grpGeneral);
    m_btnHotkey->setCursor(Qt::PointingHandCursor);
    connect(m_btnHotkey, &QPushButton::clicked, this, &MainWindow::OnHotkeyButtonClicked);
    genLayout->addWidget(m_btnHotkey, 1, 1);

    m_btnClearHotkey = new QPushButton("Clear", grpGeneral);
    connect(m_btnClearHotkey, &QPushButton::clicked, this, &MainWindow::OnClearHotkeyClicked);
    genLayout->addWidget(m_btnClearHotkey, 1, 2);

    genLayout->addWidget(new QLabel("Activation Mode:"), 2, 0);
    m_comboMode = new QComboBox(grpGeneral);
    m_comboMode->addItem("Hold (Active while key held)");
    m_comboMode->addItem("Toggle (Press to enable/disable)");
    m_comboMode->addItem("Hold Override (Priority while held)");
    connect(m_comboMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::OnActivationModeChanged);
    genLayout->addWidget(m_comboMode, 2, 1, 1, 2);

    m_chkBlockInput = new QCheckBox("Block original hotkey keypress from other apps", grpGeneral);
    connect(m_chkBlockInput, &QCheckBox::toggled, this, &MainWindow::OnBlockInputToggled);
    genLayout->addWidget(m_chkBlockInput, 3, 0, 1, 3);

    midLayout->addWidget(grpGeneral);

    // Sensitivity Scaling Group
    auto* grpSens = new QGroupBox("Sensitivity Multipliers", midContainer);
    auto* sensLayout = new QGridLayout(grpSens);
    sensLayout->setContentsMargins(12, 16, 12, 14);
    sensLayout->setHorizontalSpacing(10);
    sensLayout->setVerticalSpacing(8);

    auto AddSensRow = [&](const QString& label, int row, QSlider*& slider, QDoubleSpinBox*& spin) {
        sensLayout->addWidget(new QLabel(label), row, 0);

        slider = new QSlider(Qt::Horizontal, grpSens);
        slider->setRange(10, 500); // 0.10x to 5.00x
        slider->setValue(100);
        sensLayout->addWidget(slider, row, 1);

        spin = new QDoubleSpinBox(grpSens);
        spin->setRange(0.10, 5.00);
        spin->setSingleStep(0.05);
        spin->setValue(1.00);
        spin->setSuffix("x");
        spin->setFixedWidth(75);
        sensLayout->addWidget(spin, row, 2);
    };

    AddSensRow("X Multiplier:", 0, m_sliderXSens, m_spinXSens);
    connect(m_sliderXSens, &QSlider::valueChanged, this, &MainWindow::OnXSensSliderChanged);
    connect(m_spinXSens, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::OnXSensSpinChanged);

    AddSensRow("Y Multiplier:", 1, m_sliderYSens, m_spinYSens);
    connect(m_sliderYSens, &QSlider::valueChanged, this, &MainWindow::OnYSensSliderChanged);
    connect(m_spinYSens, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::OnYSensSpinChanged);

    AddSensRow("Overall Sens:", 2, m_sliderOverallSens, m_spinOverallSens);
    connect(m_sliderOverallSens, &QSlider::valueChanged, this, &MainWindow::OnOverallSensSliderChanged);
    connect(m_spinOverallSens, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::OnOverallSensSpinChanged);

    midLayout->addWidget(grpSens);

    // Acceleration Engine Group
    auto* grpAccel = new QGroupBox("Mouse Acceleration Curve", midContainer);
    auto* accelLayout = new QGridLayout(grpAccel);
    accelLayout->setContentsMargins(12, 16, 12, 14);
    accelLayout->setHorizontalSpacing(10);
    accelLayout->setVerticalSpacing(8);

    m_chkAccelEnable = new QCheckBox("Enable Acceleration Curve", grpAccel);
    connect(m_chkAccelEnable, &QCheckBox::toggled, this, &MainWindow::OnAccelToggled);
    accelLayout->addWidget(m_chkAccelEnable, 0, 0, 1, 3);

    accelLayout->addWidget(new QLabel("Curve Model:"), 1, 0);
    m_comboCurveType = new QComboBox(grpAccel);
    m_comboCurveType->addItem("Linear Acceleration");
    m_comboCurveType->addItem("Polynomial Acceleration");
    m_comboCurveType->addItem("Exponential Acceleration");
    m_comboCurveType->addItem("Power Curve (Classic Quake/Source)");
    m_comboCurveType->addItem("Custom LUT (Look-up Table)");
    connect(m_comboCurveType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::OnCurveTypeChanged);
    accelLayout->addWidget(m_comboCurveType, 1, 1, 1, 2);

    // Strength
    accelLayout->addWidget(new QLabel("Strength:"), 2, 0);
    m_sliderStrength = new QSlider(Qt::Horizontal, grpAccel);
    m_sliderStrength->setRange(5, 300); // 0.05 to 3.00
    m_sliderStrength->setValue(50);
    accelLayout->addWidget(m_sliderStrength, 2, 1);
    m_spinStrength = new QDoubleSpinBox(grpAccel);
    m_spinStrength->setRange(0.05, 3.00);
    m_spinStrength->setSingleStep(0.05);
    m_spinStrength->setValue(0.50);
    m_spinStrength->setFixedWidth(75);
    accelLayout->addWidget(m_spinStrength, 2, 2);
    connect(m_sliderStrength, &QSlider::valueChanged, this, &MainWindow::OnStrengthSliderChanged);
    connect(m_spinStrength, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::OnStrengthSpinChanged);

    // Threshold
    accelLayout->addWidget(new QLabel("Threshold:"), 3, 0);
    m_sliderThreshold = new QSlider(Qt::Horizontal, grpAccel);
    m_sliderThreshold->setRange(0, 500); // 0 to 500 counts/sec
    m_sliderThreshold->setValue(50);
    accelLayout->addWidget(m_sliderThreshold, 3, 1);
    m_spinThreshold = new QSpinBox(grpAccel);
    m_spinThreshold->setRange(0, 500);
    m_spinThreshold->setSingleStep(10);
    m_spinThreshold->setValue(50);
    m_spinThreshold->setSuffix(" c/s");
    m_spinThreshold->setFixedWidth(75);
    accelLayout->addWidget(m_spinThreshold, 3, 2);
    connect(m_sliderThreshold, &QSlider::valueChanged, this, &MainWindow::OnThresholdSliderChanged);
    connect(m_spinThreshold, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::OnThresholdSpinChanged);

    // Exponent
    accelLayout->addWidget(new QLabel("Exponent:"), 4, 0);
    m_sliderExponent = new QSlider(Qt::Horizontal, grpAccel);
    m_sliderExponent->setRange(10, 400); // 0.10 to 4.00
    m_sliderExponent->setValue(200);
    accelLayout->addWidget(m_sliderExponent, 4, 1);
    m_spinExponent = new QDoubleSpinBox(grpAccel);
    m_spinExponent->setRange(0.10, 4.00);
    m_spinExponent->setSingleStep(0.10);
    m_spinExponent->setValue(2.00);
    m_spinExponent->setFixedWidth(75);
    accelLayout->addWidget(m_spinExponent, 4, 2);
    connect(m_sliderExponent, &QSlider::valueChanged, this, &MainWindow::OnExponentSliderChanged);
    connect(m_spinExponent, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::OnExponentSpinChanged);

    // Max Multiplier
    accelLayout->addWidget(new QLabel("Max Multiplier:"), 5, 0);
    m_sliderMaxMult = new QSlider(Qt::Horizontal, grpAccel);
    m_sliderMaxMult->setRange(100, 500); // 1.00x to 5.00x
    m_sliderMaxMult->setValue(250);
    accelLayout->addWidget(m_sliderMaxMult, 5, 1);
    m_spinMaxMult = new QDoubleSpinBox(grpAccel);
    m_spinMaxMult->setRange(1.00, 5.00);
    m_spinMaxMult->setSingleStep(0.10);
    m_spinMaxMult->setValue(2.50);
    m_spinMaxMult->setSuffix("x");
    m_spinMaxMult->setFixedWidth(75);
    accelLayout->addWidget(m_spinMaxMult, 5, 2);
    connect(m_sliderMaxMult, &QSlider::valueChanged, this, &MainWindow::OnMaxMultSliderChanged);
    connect(m_spinMaxMult, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::OnMaxMultSpinChanged);

    // Order
    accelLayout->addWidget(new QLabel("Pipeline Order:"), 6, 0);
    m_comboOrder = new QComboBox(grpAccel);
    m_comboOrder->addItem("Sensitivity -> Acceleration -> Output");
    m_comboOrder->addItem("Acceleration -> Sensitivity -> Output");
    connect(m_comboOrder, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::OnOrderChanged);
    accelLayout->addWidget(m_comboOrder, 6, 1, 1, 2);

    midLayout->addWidget(grpAccel);
    midLayout->addStretch();

    midScroll->setWidget(midContainer);
    mainLayout->addWidget(midScroll, 1);

    // ==========================================
    // 3. RIGHT PANEL: Curve Visualizer & Debug HUD
    // ==========================================
    auto* rightContainer = new QWidget(centralWidget);
    rightContainer->setFixedWidth(400);
    auto* rightLayout = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(10);

    auto* lblRightHeader = new QLabel("Acceleration Curve & Live HUD", rightContainer);
    lblRightHeader->setStyleSheet("font-size: 16px; font-weight: bold; color: #FFFFFF;");
    rightLayout->addWidget(lblRightHeader);

    m_curveWidget = new CurveWidget(rightContainer);
    m_curveWidget->setFixedHeight(240);
    rightLayout->addWidget(m_curveWidget);

    auto* lblHud = new QLabel("Live Telemetry & Debug Stream:", rightContainer);
    lblHud->setStyleSheet("font-size: 12px; font-weight: bold; color: #888888;");
    rightLayout->addWidget(lblHud);

    m_hudText = new QTextEdit(rightContainer);
    m_hudText->setReadOnly(true);
    rightLayout->addWidget(m_hudText, 1);

    m_btnEmergency = new QPushButton("Emergency Killswitch: OFF (Ctrl+Shift+F12)", rightContainer);
    m_btnEmergency->setFixedHeight(40);
    m_btnEmergency->setStyleSheet(
        "background-color: #242424; border: 1px solid #383838; font-weight: bold; color: #E0E0E0; border-radius: 4px;"
    );
    connect(m_btnEmergency, &QPushButton::clicked, this, &MainWindow::OnEmergencyToggleClicked);
    rightLayout->addWidget(m_btnEmergency);

    mainLayout->addWidget(rightContainer);
}

void MainWindow::RefreshProfileList() {
    m_isUpdatingUi = true;
    m_profileList->clear();

    const auto& profiles = m_profileManager.GetProfiles();
    int selRow = 0;

    for (int i = 0; i < static_cast<int>(profiles.size()); ++i) {
        QString name = QString::fromStdString(profiles[i].name);
        if (profiles[i].id == m_profileManager.GetDefaultProfileId()) {
            name += " [Default]";
        }
        m_profileList->addItem(name);
        if (profiles[i].id == m_selectedProfileId) {
            selRow = i;
        }
    }

    if (!profiles.empty()) {
        if (m_selectedProfileId.empty()) {
            m_selectedProfileId = profiles.front().id;
            selRow = 0;
        }
        m_profileList->setCurrentRow(selRow);
        LoadProfileToUi(m_selectedProfileId);
    }
    m_isUpdatingUi = false;
}

void MainWindow::LoadProfileToUi(const std::string& profileId) {
    auto pOpt = m_profileManager.GetProfileById(profileId);
    if (!pOpt) return;

    m_isUpdatingUi = true;
    const Profile& p = *pOpt;
    m_selectedProfileId = p.id;

    m_editName->setText(QString::fromStdString(p.name));

    std::string hkStr = p.hotkey.ToString();
    if (hkStr == "None") hkStr = "Bind Hotkey...";
    m_btnHotkey->setText(QString::fromStdString(hkStr));
    m_btnHotkey->setStyleSheet("");

    m_comboMode->setCurrentIndex(static_cast<int>(p.activationMode));
    m_chkBlockInput->setChecked(p.blockOriginalHotkeyInput);

    // Sensitivities
    m_sliderXSens->setValue(static_cast<int>(std::round(p.xMultiplier * 100.0)));
    m_spinXSens->setValue(p.xMultiplier);

    m_sliderYSens->setValue(static_cast<int>(std::round(p.yMultiplier * 100.0)));
    m_spinYSens->setValue(p.yMultiplier);

    m_sliderOverallSens->setValue(static_cast<int>(std::round(p.overallMultiplier * 100.0)));
    m_spinOverallSens->setValue(p.overallMultiplier);

    // Acceleration
    m_chkAccelEnable->setChecked(p.acceleration.enabled);
    m_comboCurveType->setCurrentIndex(static_cast<int>(p.acceleration.type));

    m_sliderStrength->setValue(static_cast<int>(std::round(p.acceleration.strength * 100.0)));
    m_spinStrength->setValue(p.acceleration.strength);

    m_sliderThreshold->setValue(static_cast<int>(std::round(p.acceleration.threshold)));
    m_spinThreshold->setValue(static_cast<int>(std::round(p.acceleration.threshold)));

    m_sliderExponent->setValue(static_cast<int>(std::round(p.acceleration.exponent * 100.0)));
    m_spinExponent->setValue(p.acceleration.exponent);

    m_sliderMaxMult->setValue(static_cast<int>(std::round(p.acceleration.maxMultiplier * 100.0)));
    m_spinMaxMult->setValue(p.acceleration.maxMultiplier);

    m_comboOrder->setCurrentIndex(static_cast<int>(p.order));

    UpdateCurveVisualizer();
    UpdateEmergencyButtonState();

    m_isUpdatingUi = false;
}

void MainWindow::SaveUiToCurrentProfile() {
    if (m_isUpdatingUi || m_selectedProfileId.empty()) return;

    auto pOpt = m_profileManager.GetProfileById(m_selectedProfileId);
    if (!pOpt) return;

    Profile p = *pOpt;
    p.name = m_editName->text().toStdString();
    p.activationMode = static_cast<ActivationMode>(m_comboMode->currentIndex());
    p.blockOriginalHotkeyInput = m_chkBlockInput->isChecked();

    p.xMultiplier = m_spinXSens->value();
    p.yMultiplier = m_spinYSens->value();
    p.overallMultiplier = m_spinOverallSens->value();

    p.acceleration.enabled = m_chkAccelEnable->isChecked();
    p.acceleration.type = static_cast<CurveType>(m_comboCurveType->currentIndex());
    p.acceleration.strength = m_spinStrength->value();
    p.acceleration.threshold = static_cast<double>(m_spinThreshold->value());
    p.acceleration.exponent = m_spinExponent->value();
    p.acceleration.maxMultiplier = m_spinMaxMult->value();

    p.order = static_cast<TransformationOrder>(m_comboOrder->currentIndex());

    m_profileManager.UpdateProfile(p);
    m_profileManager.SetManualBaseProfile(p.id);

    UpdateCurveVisualizer();
}

void MainWindow::UpdateCurveVisualizer() {
    if (!m_curveWidget) return;

    auto pOpt = m_profileManager.GetProfileById(m_selectedProfileId);
    if (!pOpt) return;

    const Profile& p = *pOpt;
    if (p.acceleration.enabled) {
        m_curveWidget->SetCurve(CreateAccelerationCurve(p.acceleration));
        m_curveWidget->SetMaxDisplayMultiplier(std::max(3.0, p.acceleration.maxMultiplier * 1.1));
    } else {
        AccelerationSettings flatLinear;
        flatLinear.type = CurveType::Linear;
        flatLinear.strength = 0.0;
        flatLinear.threshold = 99999.0;
        flatLinear.maxMultiplier = 1.0;
        m_curveWidget->SetCurve(CreateAccelerationCurve(flatLinear));
        m_curveWidget->SetMaxDisplayMultiplier(2.0);
    }
}

void MainWindow::UpdateEmergencyButtonState() {
    if (!m_btnEmergency) return;
    bool disabled = m_profileManager.IsEmergencyDisabled();
    if (disabled) {
        m_btnEmergency->setText("Emergency Killswitch: ACTIVE (Click to Restore)");
        m_btnEmergency->setStyleSheet(
            "background-color: #B71C1C; border: 1px solid #FF5252; color: #FFFFFF; font-weight: bold; border-radius: 4px;"
        );
    } else {
        m_btnEmergency->setText("Emergency Killswitch: OFF (Ctrl+Shift+F12)");
        m_btnEmergency->setStyleSheet(
            "background-color: #222630; border: 1px solid #2B303C; color: #E1E6F0; font-weight: bold; border-radius: 4px;"
        );
    }
}

void MainWindow::StartHotkeyRecording() {
    m_isRecordingHotkey = true;
    m_btnHotkey->setText("Press any Key / Mouse Button...");
    m_btnHotkey->setStyleSheet(
        "background-color: #E2231A; color: white; font-weight: bold; border: 1px solid #FF4444;"
    );
}

void MainWindow::StopHotkeyRecording(bool apply, const Hotkey& hk) {
    m_isRecordingHotkey = false;
    if (apply && !m_selectedProfileId.empty()) {
        auto pOpt = m_profileManager.GetProfileById(m_selectedProfileId);
        if (pOpt) {
            Profile p = *pOpt;
            p.hotkey = hk;
            m_profileManager.UpdateProfile(p);
        }
    }
    m_btnHotkey->setStyleSheet("");
    if (!m_selectedProfileId.empty()) {
        LoadProfileToUi(m_selectedProfileId);
    }
}

void MainWindow::OnGlobalInputRecorded(uint32_t vkCode, MouseButton button, uint32_t modifiers) {
    if (!m_isRecordingHotkey) return;

    if (vkCode == VK_ESCAPE) {
        StopHotkeyRecording(false);
        return;
    }

    Hotkey hk;
    hk.vkCode = vkCode;
    hk.mouseButton = button;
    hk.modifiers = modifiers;

    StopHotkeyRecording(true, hk);
}

// ----------------------------------------------------
// Slots
// ----------------------------------------------------
void MainWindow::OnProfileSelectionChanged() {
    if (m_isUpdatingUi) return;
    int row = m_profileList->currentRow();
    const auto& profiles = m_profileManager.GetProfiles();
    if (row >= 0 && row < static_cast<int>(profiles.size())) {
        m_selectedProfileId = profiles[row].id;
        m_profileManager.SetManualBaseProfile(m_selectedProfileId);
        LoadProfileToUi(m_selectedProfileId);
    }
}

void MainWindow::OnNewProfileClicked() {
    Profile p = Profile::CreateDefault();
    p.id = "profile_" + std::to_string(GetTickCount64());
    p.name = "Custom Profile " + std::to_string(m_profileManager.GetProfiles().size() + 1);
    m_profileManager.AddProfile(p);
    m_selectedProfileId = p.id;
    m_profileManager.SetManualBaseProfile(p.id);
    RefreshProfileList();
}

void MainWindow::OnDuplicateProfileClicked() {
    auto pOpt = m_profileManager.GetProfileById(m_selectedProfileId);
    if (!pOpt) return;

    Profile dup = *pOpt;
    dup.id = "profile_" + std::to_string(GetTickCount64());
    dup.name += " (Copy)";
    m_profileManager.AddProfile(dup);
    m_selectedProfileId = dup.id;
    m_profileManager.SetManualBaseProfile(dup.id);
    RefreshProfileList();
}

void MainWindow::OnDeleteProfileClicked() {
    if (m_profileManager.GetProfiles().size() <= 1) {
        QMessageBox::warning(this, "SensShift", "Cannot delete the only remaining profile.");
        return;
    }

    if (m_profileManager.DeleteProfile(m_selectedProfileId)) {
        m_selectedProfileId.clear();
        RefreshProfileList();
    }
}

void MainWindow::OnSaveConfigClicked() {
    AppConfig cfg = m_configManager.GetConfig();
    cfg.profiles = m_profileManager.GetProfiles();
    cfg.defaultProfileId = m_profileManager.GetDefaultProfileId();
    m_configManager.GetConfig() = cfg;
    if (m_configManager.Save()) {
        QMessageBox::information(this, "SensShift", "Configuration successfully saved to config.json!");
    }
}

void MainWindow::OnProfileNameChanged(const QString& text) {
    if (m_isUpdatingUi) return;
    SaveUiToCurrentProfile();
    int row = m_profileList->currentRow();
    if (row >= 0 && row < m_profileList->count()) {
        m_isUpdatingUi = true;
        QString name = text;
        if (m_selectedProfileId == m_profileManager.GetDefaultProfileId()) {
            name += " [Default]";
        }
        m_profileList->item(row)->setText(name);
        m_isUpdatingUi = false;
    }
}

void MainWindow::OnHotkeyButtonClicked() {
    StartHotkeyRecording();
}

void MainWindow::OnClearHotkeyClicked() {
    StopHotkeyRecording(true, Hotkey{});
}

void MainWindow::OnActivationModeChanged(int /*index*/) {
    SaveUiToCurrentProfile();
}

void MainWindow::OnBlockInputToggled(bool /*checked*/) {
    SaveUiToCurrentProfile();
}

void MainWindow::OnXSensSliderChanged(int value) {
    if (m_isUpdatingUi) return;
    m_isUpdatingUi = true;
    m_spinXSens->setValue(value / 100.0);
    m_isUpdatingUi = false;
    SaveUiToCurrentProfile();
}

void MainWindow::OnXSensSpinChanged(double value) {
    if (m_isUpdatingUi) return;
    m_isUpdatingUi = true;
    m_sliderXSens->setValue(static_cast<int>(std::round(value * 100.0)));
    m_isUpdatingUi = false;
    SaveUiToCurrentProfile();
}

void MainWindow::OnYSensSliderChanged(int value) {
    if (m_isUpdatingUi) return;
    m_isUpdatingUi = true;
    m_spinYSens->setValue(value / 100.0);
    m_isUpdatingUi = false;
    SaveUiToCurrentProfile();
}

void MainWindow::OnYSensSpinChanged(double value) {
    if (m_isUpdatingUi) return;
    m_isUpdatingUi = true;
    m_sliderYSens->setValue(static_cast<int>(std::round(value * 100.0)));
    m_isUpdatingUi = false;
    SaveUiToCurrentProfile();
}

void MainWindow::OnOverallSensSliderChanged(int value) {
    if (m_isUpdatingUi) return;
    m_isUpdatingUi = true;
    m_spinOverallSens->setValue(value / 100.0);
    m_isUpdatingUi = false;
    SaveUiToCurrentProfile();
}

void MainWindow::OnOverallSensSpinChanged(double value) {
    if (m_isUpdatingUi) return;
    m_isUpdatingUi = true;
    m_sliderOverallSens->setValue(static_cast<int>(std::round(value * 100.0)));
    m_isUpdatingUi = false;
    SaveUiToCurrentProfile();
}

void MainWindow::OnAccelToggled(bool /*checked*/) {
    SaveUiToCurrentProfile();
}

void MainWindow::OnCurveTypeChanged(int /*index*/) {
    SaveUiToCurrentProfile();
}

void MainWindow::OnStrengthSliderChanged(int value) {
    if (m_isUpdatingUi) return;
    m_isUpdatingUi = true;
    m_spinStrength->setValue(value / 100.0);
    m_isUpdatingUi = false;
    SaveUiToCurrentProfile();
}

void MainWindow::OnStrengthSpinChanged(double value) {
    if (m_isUpdatingUi) return;
    m_isUpdatingUi = true;
    m_sliderStrength->setValue(static_cast<int>(std::round(value * 100.0)));
    m_isUpdatingUi = false;
    SaveUiToCurrentProfile();
}

void MainWindow::OnThresholdSliderChanged(int value) {
    if (m_isUpdatingUi) return;
    m_isUpdatingUi = true;
    m_spinThreshold->setValue(value);
    m_isUpdatingUi = false;
    SaveUiToCurrentProfile();
}

void MainWindow::OnThresholdSpinChanged(int value) {
    if (m_isUpdatingUi) return;
    m_isUpdatingUi = true;
    m_sliderThreshold->setValue(value);
    m_isUpdatingUi = false;
    SaveUiToCurrentProfile();
}

void MainWindow::OnExponentSliderChanged(int value) {
    if (m_isUpdatingUi) return;
    m_isUpdatingUi = true;
    m_spinExponent->setValue(value / 100.0);
    m_isUpdatingUi = false;
    SaveUiToCurrentProfile();
}

void MainWindow::OnExponentSpinChanged(double value) {
    if (m_isUpdatingUi) return;
    m_isUpdatingUi = true;
    m_sliderExponent->setValue(static_cast<int>(std::round(value * 100.0)));
    m_isUpdatingUi = false;
    SaveUiToCurrentProfile();
}

void MainWindow::OnMaxMultSliderChanged(int value) {
    if (m_isUpdatingUi) return;
    m_isUpdatingUi = true;
    m_spinMaxMult->setValue(value / 100.0);
    m_isUpdatingUi = false;
    SaveUiToCurrentProfile();
}

void MainWindow::OnMaxMultSpinChanged(double value) {
    if (m_isUpdatingUi) return;
    m_isUpdatingUi = true;
    m_sliderMaxMult->setValue(static_cast<int>(std::round(value * 100.0)));
    m_isUpdatingUi = false;
    SaveUiToCurrentProfile();
}

void MainWindow::OnOrderChanged(int /*index*/) {
    SaveUiToCurrentProfile();
}

void MainWindow::OnEmergencyToggleClicked() {
    m_profileManager.ToggleEmergencyDisable();
    UpdateEmergencyButtonState();
}

void MainWindow::OnHudTimerTimeout() {
    // IMPORTANT: These calls each use independent, non-nesting locks.
    // GetTelemetry() uses Pipeline::m_telemetryMutex (NOT Pipeline::m_mutex).
    // IsModifierActive() and IsEmergencyDisabled() use atomics (NO lock at all).
    // This eliminates the ABBA deadlock that previously froze the app.
    PipelineTelemetry tel = m_pipeline.GetTelemetry();
    bool modActive = m_profileManager.IsModifierActive();
    bool emergencyOff = m_profileManager.IsEmergencyDisabled();
    int64_t hookAgeMs = m_mouseInterceptor.GetLastEventAgeMs();

    // Update curve widget velocity marker
    if (m_curveWidget) {
        m_curveWidget->SetCurrentVelocity(tel.velocity);
    }

    // Format telemetry HUD string identically to initial commit format
    std::stringstream ss;
    ss << "=== REAL-TIME TELEMETRY ===\r\n";
    ss << "Active Profile:   " << tel.profileName;
    if (tel.isModifierActive) ss << " [MODIFIER ACTIVE]";
    if (tel.isEmergencyDisabled) ss << " [DISABLED]";
    ss << "\r\n";
    ss << "Raw Delta:        X=" << tel.rawX << ", Y=" << tel.rawY << "\r\n";
    ss << "Instant Velocity: " << std::fixed << std::setprecision(1) << tel.velocity << " counts/s\r\n";
    ss << "Accel Multiplier: " << std::fixed << std::setprecision(3) << tel.accelMultiplier << "x\r\n";
    ss << "Eff X Multiplier: " << std::fixed << std::setprecision(3) << tel.effXMultiplier << "x\r\n";
    ss << "Eff Y Multiplier: " << std::fixed << std::setprecision(3) << tel.effYMultiplier << "x\r\n";
    ss << "Transformed Out:  X=" << tel.outX << ", Y=" << tel.outY << "\r\n";
    ss << "===========================\r\n";
    ss << "Modifiers Active: " << (modActive ? "YES" : "NO") << "\r\n";
    ss << "Emergency Failsafe: " << (emergencyOff ? "ACTIVE (CTRL+SHIFT+F12)" : "OFF") << "\r\n";

    // Hook health indicator
    if (hookAgeMs < 0) {
        ss << "Mouse Hook:       Waiting for first event...\r\n";
    } else if (hookAgeMs < 2000) {
        ss << "Mouse Hook:       OK (" << hookAgeMs << "ms ago)\r\n";
    } else {
        ss << "Mouse Hook:       WARNING - no events for " << (hookAgeMs / 1000) << "s\r\n";
    }

    QString newText = QString::fromStdString(ss.str());
    std::string newStr = ss.str();
    if (newStr != m_lastHudString) {
        m_lastHudString = std::move(newStr);
        m_hudText->setPlainText(newText);
    }
}
