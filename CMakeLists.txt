cmake_minimum_required(VERSION 3.20)
project(SensShift VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH "C:/Qt/6.5.3/msvc2019_64")

# Qt standard properties for automoc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if (MSVC)
    add_compile_options(/W4 /WX- /permissive- /utf-8 /FImsvc_compat.h)
    add_compile_options("$<$<CONFIG:Release>:/O2>")
    add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX -DWIN32_LEAN_AND_MEAN)
endif()

find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/src/profile
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/qt
)

# Core engine and profile sources
set(CORE_SOURCES
    src/core/InputDefines.h
    src/core/MouseInjector.h
    src/core/MouseInjector.cpp
    src/core/KeyboardHotkeyManager.h
    src/core/KeyboardHotkeyManager.cpp
    src/core/MouseInterceptor.h
    src/core/MouseInterceptor.cpp
    src/engine/IAccelerationCurve.h
    src/engine/Curves.h
    src/engine/Curves.cpp
    src/engine/FractionalAccumulator.h
    src/engine/TransformationPipeline.h
    src/engine/TransformationPipeline.cpp
    src/profile/Profile.h
    src/profile/Profile.cpp
    src/profile/ProfileManager.h
    src/profile/ProfileManager.cpp
    src/profile/AppTracker.h
    src/profile/AppTracker.cpp
    src/profile/ConfigManager.h
    src/profile/ConfigManager.cpp
)

set(QT_UI_SOURCES
    src/ui/qt/CurveWidget.h
    src/ui/qt/CurveWidget.cpp
    src/ui/qt/MainWindow.h
    src/ui/qt/MainWindow.cpp
)

# Main Application
add_executable(SensShift WIN32
    src/main.cpp
    ${CORE_SOURCES}
    ${QT_UI_SOURCES}
)

target_link_libraries(SensShift PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    user32
    gdi32
    shell32
    shlwapi
    dwmapi
    ole32
)

# Unit / Engine test executable
add_executable(SensEngineTests
    tests/SensEngineTests.cpp
    ${CORE_SOURCES}
)

target_link_libraries(SensEngineTests PRIVATE
    user32
    gdi32
    shell32
    shlwapi
    ole32
)
