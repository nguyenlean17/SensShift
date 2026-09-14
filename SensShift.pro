QT += core gui widgets
CONFIG += c++20

TARGET = SensShift
TEMPLATE = app

INCLUDEPATH += src src/core src/engine src/profile src/ui/qt

SOURCES += \
    src/main.cpp \
    src/core/MouseInjector.cpp \
    src/core/KeyboardHotkeyManager.cpp \
    src/core/MouseInterceptor.cpp \
    src/engine/Curves.cpp \
    src/engine/TransformationPipeline.cpp \
    src/profile/Profile.cpp \
    src/profile/ProfileManager.cpp \
    src/profile/AppTracker.cpp \
    src/profile/ConfigManager.cpp \
    src/ui/qt/CurveWidget.cpp \
    src/ui/qt/MainWindow.cpp

HEADERS += \
    src/msvc_compat.h \
    src/core/InputDefines.h \
    src/core/MouseInjector.h \
    src/core/KeyboardHotkeyManager.h \
    src/core/MouseInterceptor.h \
    src/engine/IAccelerationCurve.h \
    src/engine/Curves.h \
    src/engine/FractionalAccumulator.h \
    src/engine/TransformationPipeline.h \
    src/profile/Profile.h \
    src/profile/ProfileManager.h \
    src/profile/AppTracker.h \
    src/profile/ConfigManager.h \
    src/profile/json.hpp \
    src/ui/qt/CurveWidget.h \
    src/ui/qt/MainWindow.h

LIBS += -luser32 -lgdi32 -lshell32 -lshlwapi -ldwmapi -lole32
