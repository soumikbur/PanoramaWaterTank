// main.cpp
#include <QFont>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "Logger.h"
#include "SettingsManager.h"
#include "TankModel.h"
#include "TimeManager.h"
#include "ConnectionManager.h"

int main(int argc, char *argv[])
{
    // High-DPI scaling is on by default in Qt 6, but the rounding policy
    // itself defaults to rounding to the nearest integer factor, which
    // can look inconsistent across a mixed-DPI multi-monitor setup or
    // Windows' common fractional scale factors (125%, 150%). PassThrough
    // uses the exact scale factor instead, which is the more predictable
    // choice for a desktop app expected to be dragged between monitors.
    // Must be set before QGuiApplication is constructed.
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("Panorama Electronics"));
    app.setOrganizationDomain(QStringLiteral("panorama-electronics.example"));
    app.setApplicationName(QStringLiteral("Panorama Water Tank Monitor"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));

           // Matches qml/theme/Typography.qml's fontFamily so native Qt Quick
           // Controls (e.g. LevelTrendCard's ComboBox popup) request the same
           // font as QML Text elements, rather than two different fonts
           // silently falling back to two different system defaults.
    QFont defaultFont(QStringLiteral("Inter"));
    QGuiApplication::setFont(defaultFont);

    Panorama::Logger logger;

           // SettingsManager loads Base URL, API Token, Device Label, etc. from QSettings in its constructor.
    Panorama::SettingsManager settings(logger);


    Panorama::ConnectionManager connectionManager(logger);

           // Local objects here are destroyed in reverse declaration order once
           // app.exec() returns: engine (and everything the QML engine owns,
           // including the TankModel singleton resolved below) is torn down
           // *before* connectionManager. ConnectionManager's own
           // connectionStateChanged/networkError handlers (ConnectionManager.cpp)
           // guard with `if (m_tankModel)`, but that only catches a null pointer,
           // not a dangling one left behind by the engine's teardown - if
           // ApiClient has an in-flight request that completes or errors during
           // shutdown, that handler could still fire against an already-destroyed
           // TankModel. Stopping polling here, while the event loop (and
           // therefore the engine and TankModel) is still fully alive, closes
           // that window instead of relying on destruction-order timing.
    QObject::connect(&app, &QCoreApplication::aboutToQuit,
                     &connectionManager, &Panorama::ConnectionManager::stop);

    QQmlApplicationEngine engine;

           // Provide the settings manager to QML so the Settings page can bind to it.
    engine.rootContext()->setContextProperty(QStringLiteral("Settings"), &settings);

           // Load the root QML component.
    engine.loadFromModule("PanoramaWaterTank", "App");

    if (engine.rootObjects().isEmpty()) {
        logger.logError(
            Panorama::Logger::LogLevel::Critical,
            QStringLiteral("Failed to load App.qml. Exiting."));
        return -1;
    }

           // Wiring
    const int tankModelTypeId =
        qmlTypeId("PanoramaWaterTank", 1, 0, "TankModel");

    auto *tankModel =
        tankModelTypeId >= 0
            ? engine.singletonInstance<Panorama::TankModel *>(tankModelTypeId)
            : nullptr;

    if (tankModel) {

        // Initialize the QML model with stored settings before enabling the backend.
        tankModel->setTankName(settings.tankName());
        tankModel->setTankRadius(settings.tankRadius());
        tankModel->setTankHeight(settings.tankHeight());
        tankModel->setDeviceLabel(settings.deviceLabel());

        connectionManager.setTankModel(tankModel);

               // Keep model geometry in sync with user settings changes.
               // Captures are explicit rather than [&]: tankModel is captured
               // by value (it's a pointer - the pointed-to object's lifetime is
               // what matters, and that's already protected below by using it
               // as the connection's context object), settings by reference
               // since SettingsManager is non-copyable and its current value
               // is what each handler needs to read.
        QObject::connect(
            &settings,
            &Panorama::SettingsManager::tankNameChanged,
            tankModel,
            [tankModel, &settings]() {
                tankModel->setTankName(settings.tankName());
            });

        QObject::connect(
            &settings,
            &Panorama::SettingsManager::tankRadiusChanged,
            tankModel,
            [tankModel, &settings]() {
                tankModel->setTankRadius(settings.tankRadius());
            });

        QObject::connect(
            &settings,
            &Panorama::SettingsManager::tankHeightChanged,
            tankModel,
            [tankModel, &settings]() {
                tankModel->setTankHeight(settings.tankHeight());
            });

        QObject::connect(
            &settings,
            &Panorama::SettingsManager::deviceLabelChanged,
            tankModel,
            [tankModel, &settings]() {
                tankModel->setDeviceLabel(settings.deviceLabel());
            });

               // Initialize ApiClient through ConnectionManager before starting.
               // Captures explicit: both connectionManager and settings are
               // non-copyable QObjects, so reference capture is required for
               // either of them regardless of style.
        auto updateApiConfig = [&connectionManager, &settings]() {
            connectionManager.setApiConfiguration(
                settings.apiBaseUrl(),
                settings.apiToken(),
                settings.deviceLabel(),
                settings.refreshIntervalMs(),
                settings.timeoutMs(),
                settings.levelVariable(),
                settings.temperatureVariable(),
                settings.pressureVariable(),
                settings.statusVariable());
        };

               // Initial configuration
        updateApiConfig();

               // Live configuration updates
        QObject::connect(&settings, &Panorama::SettingsManager::apiBaseUrlChanged,
                         &connectionManager, updateApiConfig);

        QObject::connect(&settings, &Panorama::SettingsManager::apiTokenChanged,
                         &connectionManager, updateApiConfig);

        QObject::connect(&settings, &Panorama::SettingsManager::deviceLabelChanged,
                         &connectionManager, updateApiConfig);

        QObject::connect(&settings, &Panorama::SettingsManager::refreshIntervalMsChanged,
                         &connectionManager, updateApiConfig);

        QObject::connect(&settings, &Panorama::SettingsManager::timeoutMsChanged,
                         &connectionManager, updateApiConfig);

        QObject::connect(&settings, &Panorama::SettingsManager::levelVariableChanged,
                         &connectionManager, updateApiConfig);

        QObject::connect(&settings, &Panorama::SettingsManager::temperatureVariableChanged,
                         &connectionManager, updateApiConfig);

        QObject::connect(&settings, &Panorama::SettingsManager::pressureVariableChanged,
                         &connectionManager, updateApiConfig);

        QObject::connect(&settings, &Panorama::SettingsManager::statusVariableChanged,
                         &connectionManager, updateApiConfig);

               // Start polling after configuration is complete.
        connectionManager.start();

    } else {

        logger.logError(
            Panorama::Logger::LogLevel::Error,
            QStringLiteral("main: failed to resolve the TankModel QML singleton"));
    }

    const int timeManagerTypeId =
        qmlTypeId("PanoramaWaterTank", 1, 0, "TimeManager");

    auto *timeManager =
        timeManagerTypeId >= 0
            ? engine.singletonInstance<Panorama::TimeManager *>(timeManagerTypeId)
            : nullptr;

    if (!timeManager) {
        logger.logError(
            Panorama::Logger::LogLevel::Error,
            QStringLiteral("main: failed to resolve the TimeManager QML singleton"));
    }

    logger.logApplication(
        Panorama::Logger::LogLevel::Info,
        QStringLiteral("Application started"));

    return app.exec();
}
