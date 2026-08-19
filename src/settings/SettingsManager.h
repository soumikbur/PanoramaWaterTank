#pragma once

#include <QObject>
#include <QSettings>
#include <QString>

namespace Panorama {

class Logger;

class SettingsManager : public QObject
{
    Q_OBJECT

           // API
    Q_PROPERTY(QString apiBaseUrl READ apiBaseUrl WRITE setApiBaseUrl NOTIFY apiBaseUrlChanged)
    Q_PROPERTY(QString apiToken READ apiToken WRITE setApiToken NOTIFY apiTokenChanged)
    Q_PROPERTY(QString deviceLabel READ deviceLabel WRITE setDeviceLabel NOTIFY deviceLabelChanged)
    Q_PROPERTY(QString dataSourceId READ dataSourceId WRITE setDataSourceId NOTIFY dataSourceIdChanged)

           // Telemetry Variables
    Q_PROPERTY(QString levelVariable READ levelVariable WRITE setLevelVariable NOTIFY levelVariableChanged)
    Q_PROPERTY(QString volumeVariable READ volumeVariable WRITE setVolumeVariable NOTIFY volumeVariableChanged)
    Q_PROPERTY(QString distanceVariable READ distanceVariable WRITE setDistanceVariable NOTIFY distanceVariableChanged)
    Q_PROPERTY(QString temperatureVariable READ temperatureVariable WRITE setTemperatureVariable NOTIFY temperatureVariableChanged)
    Q_PROPERTY(QString pressureVariable READ pressureVariable WRITE setPressureVariable NOTIFY pressureVariableChanged)
    Q_PROPERTY(QString statusVariable READ statusVariable WRITE setStatusVariable NOTIFY statusVariableChanged)

    Q_PROPERTY(QString readingType READ readingType WRITE setReadingType NOTIFY readingTypeChanged)
    Q_PROPERTY(double sensorMountingOffset READ sensorMountingOffset WRITE setSensorMountingOffset NOTIFY sensorMountingOffsetChanged)

    Q_PROPERTY(int refreshIntervalMs READ refreshIntervalMs WRITE setRefreshIntervalMs NOTIFY refreshIntervalMsChanged)
    Q_PROPERTY(int timeoutMs READ timeoutMs WRITE setTimeoutMs NOTIFY timeoutMsChanged)

           // Tank Configuration
    Q_PROPERTY(QString tankName READ tankName WRITE setTankName NOTIFY tankNameChanged)
    Q_PROPERTY(double tankRadius READ tankRadius WRITE setTankRadius NOTIFY tankRadiusChanged)
    Q_PROPERTY(double tankHeight READ tankHeight WRITE setTankHeight NOTIFY tankHeightChanged)

           // Logging
    Q_PROPERTY(QString logLevel READ logLevel WRITE setLogLevel NOTIFY logLevelChanged)
    Q_PROPERTY(int logRetentionDays READ logRetentionDays WRITE setLogRetentionDays NOTIFY logRetentionDaysChanged)

           // Theme
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)

public:
    explicit SettingsManager(Logger &logger, QObject *parent = nullptr);

    QString apiBaseUrl() const { return m_apiBaseUrl; }
    QString apiToken() const { return m_apiToken; }
    QString deviceLabel() const { return m_deviceLabel; }
    QString dataSourceId() const { return m_dataSourceId; }

    QString levelVariable() const { return m_levelVariable; }
    QString volumeVariable() const { return m_volumeVariable; }
    QString distanceVariable() const { return m_distanceVariable; }
    QString temperatureVariable() const { return m_temperatureVariable; }
    QString pressureVariable() const { return m_pressureVariable; }
    QString statusVariable() const { return m_statusVariable; }

    QString readingType() const { return m_readingType; }
    double sensorMountingOffset() const { return m_sensorMountingOffset; }

    int refreshIntervalMs() const { return m_refreshIntervalMs; }
    int timeoutMs() const { return m_timeoutMs; }

    QString tankName() const { return m_tankName; }
    double tankRadius() const { return m_tankRadius; }
    double tankHeight() const { return m_tankHeight; }

    QString logLevel() const { return m_logLevel; }
    int logRetentionDays() const { return m_logRetentionDays; }

    QString theme() const { return m_theme; }

public slots:
    void setApiBaseUrl(const QString &url);
    void setApiToken(const QString &token);
    void setDeviceLabel(const QString &label);
    void setDataSourceId(const QString &id);

    void setLevelVariable(const QString &variable);
    void setVolumeVariable(const QString &variable);
    void setDistanceVariable(const QString &variable);
    void setTemperatureVariable(const QString &variable);
    void setPressureVariable(const QString &variable);
    void setStatusVariable(const QString &variable);

    void setReadingType(const QString &type);
    void setSensorMountingOffset(double offset);

    void setRefreshIntervalMs(int ms);
    void setTimeoutMs(int ms);

    void setTankName(const QString &name);
    void setTankRadius(double meters);
    void setTankHeight(double meters);

    void setLogLevel(const QString &level);
    void setLogRetentionDays(int days);

    void setTheme(const QString &theme);

signals:
    void apiBaseUrlChanged();
    void apiTokenChanged();
    void deviceLabelChanged();
    void dataSourceIdChanged();
    void levelVariableChanged();
    void volumeVariableChanged();
    void distanceVariableChanged();
    void temperatureVariableChanged();
    void pressureVariableChanged();
    void statusVariableChanged();
    void readingTypeChanged();
    void sensorMountingOffsetChanged();
    void refreshIntervalMsChanged();
    void timeoutMsChanged();

    void tankNameChanged();
    void tankRadiusChanged();
    void tankHeightChanged();

    void logLevelChanged();
    void logRetentionDaysChanged();

    void themeChanged();

private:
    void loadAll();
    void rejected(const QString &settingName, const QString &reason) const;
    void accepted(const QString &settingName) const;

    static bool isValidReadingType(const QString &value);
    static bool isValidLogLevel(const QString &value);
    static bool isValidTheme(const QString &value);

    Logger &m_logger;
    QSettings m_settings;

    QString m_apiBaseUrl;
    QString m_apiToken;
    QString m_deviceLabel;
    QString m_dataSourceId;

    QString m_levelVariable;
    QString m_volumeVariable;
    QString m_distanceVariable;
    QString m_temperatureVariable;
    QString m_pressureVariable;
    QString m_statusVariable;

    QString m_readingType;
    double m_sensorMountingOffset = 0.0;

    int m_refreshIntervalMs = 15000;
    int m_timeoutMs = 10000;

    QString m_tankName;
    double m_tankRadius = 0.11283791670955126;
    double m_tankHeight = 0.50;

    QString m_logLevel;
    int m_logRetentionDays = 7;

    QString m_theme;
};

} // namespace Panorama