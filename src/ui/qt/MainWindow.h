#pragma once

#include <QMainWindow>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLabel>
#include <QTextEdit>
#include <QTimer>
#include <memory>
#include <string>

#include "ProfileManager.h"
#include "ConfigManager.h"
#include "TransformationPipeline.h"
#include "MouseInterceptor.h"
#include "KeyboardHotkeyManager.h"
#include "CurveWidget.h"
#include "Curves.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(ProfileManager& profileManager,
               ConfigManager& configManager,
               TransformationPipeline& pipeline,
               MouseInterceptor& mouseInterceptor,
               KeyboardHotkeyManager& keyboardManager,
               QWidget* parent = nullptr);
    ~MainWindow() override;

    // Called when user presses a key/button during hotkey recording
    void OnGlobalInputRecorded(uint32_t vkCode, MouseButton button, uint32_t modifiers);

private slots:
    void OnProfileSelectionChanged();
    void OnNewProfileClicked();
    void OnDuplicateProfileClicked();
    void OnDeleteProfileClicked();
    void OnSaveConfigClicked();

    void OnProfileNameChanged(const QString& text);
    void OnHotkeyButtonClicked();
    void OnClearHotkeyClicked();
    void OnActivationModeChanged(int index);
    void OnBlockInputToggled(bool checked);

    void OnXSensSliderChanged(int value);
    void OnXSensSpinChanged(double value);
    void OnYSensSliderChanged(int value);
    void OnYSensSpinChanged(double value);
    void OnOverallSensSliderChanged(int value);
    void OnOverallSensSpinChanged(double value);

    void OnAccelToggled(bool checked);
    void OnCurveTypeChanged(int index);
    void OnStrengthSliderChanged(int value);
    void OnStrengthSpinChanged(double value);
    void OnThresholdSliderChanged(int value);
    void OnThresholdSpinChanged(int value);
    void OnExponentSliderChanged(int value);
    void OnExponentSpinChanged(double value);
    void OnMaxMultSliderChanged(int value);
    void OnMaxMultSpinChanged(double value);

    void OnOrderChanged(int index);

    void OnEmergencyToggleClicked();
    void OnHudTimerTimeout();

private:
    void SetupUi();
    void ApplyDarkStyle();
    void RefreshProfileList();
    void LoadProfileToUi(const std::string& profileId);
    void SaveUiToCurrentProfile();
    void UpdateEmergencyButtonState();
    void UpdateCurveVisualizer();
    void StartHotkeyRecording();
    void StopHotkeyRecording(bool apply, const Hotkey& hk = {});

    ProfileManager& m_profileManager;
    ConfigManager& m_configManager;
    TransformationPipeline& m_pipeline;
    MouseInterceptor& m_mouseInterceptor;
    KeyboardHotkeyManager& m_keyboardManager;

    std::string m_selectedProfileId;
    bool m_isUpdatingUi = false;
    bool m_isRecordingHotkey = false;
    std::string m_lastHudString;

    // Left Panel: Profiles
    QListWidget* m_profileList = nullptr;
    QPushButton* m_btnNew = nullptr;
    QPushButton* m_btnDup = nullptr;
    QPushButton* m_btnDel = nullptr;
    QPushButton* m_btnSave = nullptr;

    // Middle Panel: Configuration
    QLineEdit* m_editName = nullptr;
    QPushButton* m_btnHotkey = nullptr;
    QPushButton* m_btnClearHotkey = nullptr;
    QComboBox* m_comboMode = nullptr;
    QCheckBox* m_chkBlockInput = nullptr;

    QSlider* m_sliderXSens = nullptr;
    QDoubleSpinBox* m_spinXSens = nullptr;
    QSlider* m_sliderYSens = nullptr;
    QDoubleSpinBox* m_spinYSens = nullptr;
    QSlider* m_sliderOverallSens = nullptr;
    QDoubleSpinBox* m_spinOverallSens = nullptr;

    QCheckBox* m_chkAccelEnable = nullptr;
    QComboBox* m_comboCurveType = nullptr;

    QSlider* m_sliderStrength = nullptr;
    QDoubleSpinBox* m_spinStrength = nullptr;
    QSlider* m_sliderThreshold = nullptr;
    QSpinBox* m_spinThreshold = nullptr;
    QSlider* m_sliderExponent = nullptr;
    QDoubleSpinBox* m_spinExponent = nullptr;
    QSlider* m_sliderMaxMult = nullptr;
    QDoubleSpinBox* m_spinMaxMult = nullptr;

    QComboBox* m_comboOrder = nullptr;

    // Right Panel: Visualizer & Debug HUD
    CurveWidget* m_curveWidget = nullptr;
    QTextEdit* m_hudText = nullptr;
    QPushButton* m_btnEmergency = nullptr;

    QTimer* m_hudTimer = nullptr;

    static MainWindow* s_instance;
};
