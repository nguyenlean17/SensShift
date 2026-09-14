#include <QApplication>
#include "ui/qt/MainWindow.h"
#include "KeyboardHotkeyManager.h"
#include "MouseInterceptor.h"
#include "MouseInjector.h"
#include "TransformationPipeline.h"
#include "ProfileManager.h"
#include "ConfigManager.h"
#include "AppTracker.h"
#include <windows.h>
#include <objbase.h>

int main(int argc, char* argv[]) {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    QApplication app(argc, argv);
    app.setApplicationName("SensShift");
    app.setOrganizationName("SensShift");

    ConfigManager configManager;
    configManager.Load();

    TransformationPipeline pipeline;
    ProfileManager profileManager(pipeline);
    profileManager.Initialize(configManager.GetConfig());

    MouseInjector injector;
    KeyboardHotkeyManager keyboardManager(profileManager);
    MouseInterceptor mouseInterceptor(profileManager, pipeline, injector, keyboardManager);
    AppTracker appTracker;

    // Start background application tracking for per-app profiles
    appTracker.Start([&](const std::string& appName) {
        profileManager.OnForegroundAppChanged(appName);
    });

    // Start low-level hooks
    keyboardManager.Start();
    mouseInterceptor.Start();

    MainWindow mainWindow(profileManager, configManager, pipeline, mouseInterceptor, keyboardManager);
    mainWindow.show();

    int result = app.exec();

    // Clean RAII teardown
    mouseInterceptor.Stop();
    keyboardManager.Stop();
    appTracker.Stop();

    // Persist final config
    AppConfig cfg = configManager.GetConfig();
    cfg.profiles = profileManager.GetProfiles();
    cfg.defaultProfileId = profileManager.GetDefaultProfileId();
    configManager.GetConfig() = cfg;
    configManager.Save();

    CoUninitialize();
    return result;
}
