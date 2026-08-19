#include "SettingsManager.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>

namespace Panorama {

namespace {
constexpr auto kApiBaseUrl = "api/baseUrl";
constexpr auto kApiToken = "api/token";
constexpr auto kDeviceLabel = "api/deviceLabel";
constexpr auto kDataSourceId = "api/dataSourceId";
constexpr auto kLevelVariable = "api/levelVariable";
constexpr auto kVolumeVariable = "api/volumeVariable";
constexpr auto kDistanceVariable = "api/distanceVariable";
constexpr auto kTemperatureVariable = "api/temperatureVariable";
constexpr auto kPressureVariable = "api/pressureVariable";
constexpr auto kStatusVariable = "api/statusVariable";
constexpr auto kReadingType = "api/readingType";
constexpr auto kSensorMountingOffset = "api/sensorMountingOffset";
constexpr auto kRefreshIntervalMs = "api/refreshIntervalMs";
constexpr auto kTimeoutMs = "api/timeoutMs";

constexpr auto kTankName = "tank/name";
constexpr auto kTankRadius = "tank/radius";
constexpr auto kTankHeight = "tank/height";

constexpr auto kLogLevel = "logging/level";
constexpr auto kLogRetentionDays = "logging/retentionDays";

constexpr auto kTheme = "theme/current";
} // namespace

SettingsManager::SettingsManager(Logger &logger, QObject *parent)
    : QObject(parent)
      , m_logger(logger)
      , m_settings(QStringLiteral("Panorama Electronics"), QStringLiteral("Panorama Water Tank Monitor"))
{
    loadAll();
}

void SettingsManager::loadAll()
{
    m_apiBaseUrl = m_settings.value(kApiBaseUrl, QStringLiteral("https://industrial.api.ubidots.com/")).toString();
    m_apiToken = m_settings.value(kApiToken, QStringLiteral("BBUS-R1UePwaJ2wFg2pKlYiArsPsmMWZvzS")).toString();
    if (m_apiToken.trimmed().isEmpty()) {
        m_apiToken = QStringLiteral("BBUS-R1UePwaJ2wFg2pKlYiArsPsmMWZvzS");
    }
    m_deviceLabel = m_settings.value(kDeviceLabel, QStringLiteral("wli")).toString();
    m_dataSourceId = m_settings.value(kDataSourceId, QStringLiteral("6a3a275d4fd32b529b38a792")).toString();

    m_levelVariable = m_settings.value(kLevelVariable, QStringLiteral("waterlevel")).toString();
    m_volumeVariable = m_settings.value(kVolumeVariable, QStringLiteral("volume")).toString();
    m_distanceVariable = m_settings.value(kDistanceVariable, QStringLiteral("distance")).toString();
    m_temperatureVariable = m_settings.value(kTemperatureVariable, QStringLiteral("")).toString();
    m_pressureVariable = m_settings.value(kPressureVariable, QStringLiteral("pressure")).toString();
    m_statusVariable = m_settings.value(kStatusVariable, QStringLiteral("sensorstatus")).toString();

    m_readingType = m_settings.value(kReadingType, QStringLiteral("Level")).toString();
    m_sensorMountingOffset = m_settings.value(kSensorMountingOffset, 0.0).toDouble();

    m_refreshIntervalMs = m_settings.value(kRefreshIntervalMs, 5000).toInt();
    m_timeoutMs = m_settings.value(kTimeoutMs, 10000).toInt();


    constexpr double kDefaultTankRadius = 0.11283791670955126; // r for 20 L capacity with 50 cm height (Area = 0.04 m^2)
    constexpr double kDefaultTankHeight = 0.50;                // 50 cm (0.50 m)

    m_tankName = m_settings.value(kTankName, QStringLiteral("Panorama Water Tank")).toString();
    m_tankRadius = m_settings.value(kTankRadius, kDefaultTankRadius).toDouble();
    m_tankHeight = m_settings.value(kTankHeight, kDefaultTankHeight).toDouble();

           // Enforce 50 cm (0.50 m) demonstration tank height and 20 L capacity baseline
    if (m_tankHeight != kDefaultTankHeight) {
        m_tankHeight = kDefaultTankHeight;
        m_settings.setValue(kTankHeight, kDefaultTankHeight);
    }
    if (qAbs(m_tankRadius - kDefaultTankRadius) > 0.0001) {
        m_tankRadius = kDefaultTankRadius;
        m_settings.setValue(kTankRadius, kDefaultTankRadius);
    }

    m_logLevel = m_settings.value(kLogLevel, QStringLiteral("Info")).toString();
    m_logRetentionDays = m_settings.value(kLogRetentionDays, 7).toInt();

    m_theme = m_settings.value(kTheme, QStringLiteral("Dark")).toString();
}

void SettingsManager::rejected(const QString &settingName, const QString &reason) const
{
    m_logger.logError(Logger::LogLevel::Warning, QStringLiteral("Settings: Rejected update to %1 (%2)").arg(settingName, reason));
}

void SettingsManager::accepted(const QString &settingName) const
{
    m_logger.logApplication(Logger::LogLevel::Debug, QStringLiteral("Settings: Updated %1").arg(settingName));
}

bool SettingsManager::isValidReadingType(const QString &value)
{
    return value == QLatin1String("Level") ||
           value == QLatin1String("Volume") ||
           value == QLatin1String("Distance");
}

bool SettingsManager::isValidLogLevel(const QString &value)
{
    return value == QLatin1String("Debug") ||
           value == QLatin1String("Info") ||
           value == QLatin1String("Warning") ||
           value == QLatin1String("Error");
}

bool SettingsManager::isValidTheme(const QString &value)
{
    return value == QLatin1String("Light") || value == QLatin1String("Dark");
}

// --- API group ---------------------------------------------------------

void SettingsManager::setApiBaseUrl(const QString &url)
{
    const QString trimmed = url.trimmed();
    if (trimmed.isEmpty()) {
        rejected(QStringLiteral("apiBaseUrl"), QStringLiteral("cannot be empty"));
        return;
    }
    if (m_apiBaseUrl == trimmed) return;
    m_apiBaseUrl = trimmed;
    m_settings.setValue(kApiBaseUrl, trimmed);
    accepted(QStringLiteral("apiBaseUrl"));
    emit apiBaseUrlChanged();
}

void SettingsManager::setApiToken(const QString &token)
{
    const QString trimmed = token.trimmed();
    if (m_apiToken == trimmed) return;
    m_apiToken = trimmed;
    m_settings.setValue(kApiToken, trimmed);
    accepted(QStringLiteral("apiToken"));
    emit apiTokenChanged();
}

void SettingsManager::setDeviceLabel(const QString &label)
{
    const QString trimmed = label.trimmed();
    if (trimmed.isEmpty()) {
        rejected(QStringLiteral("deviceLabel"), QStringLiteral("cannot be empty"));
        return;
    }
    if (m_deviceLabel == trimmed) return;
    m_deviceLabel = trimmed;
    m_settings.setValue(kDeviceLabel, trimmed);
    accepted(QStringLiteral("deviceLabel"));
    emit deviceLabelChanged();
}

void SettingsManager::setDataSourceId(const QString &id)
{
    const QString trimmed = id.trimmed();
    if (trimmed.isEmpty()) {
        rejected(QStringLiteral("dataSourceId"), QStringLiteral("cannot be empty"));
        return;
    }
    if (m_dataSourceId == trimmed) return;
    m_dataSourceId = trimmed;
    m_settings.setValue(kDataSourceId, trimmed);
    accepted(QStringLiteral("dataSourceId"));
    emit dataSourceIdChanged();
}

void SettingsManager::setLevelVariable(const QString &variable)
{
    const QString trimmed = variable.trimmed();
    if (trimmed.isEmpty()) {
        rejected(QStringLiteral("levelVariable"), QStringLiteral("cannot be empty"));
        return;
    }
    if (m_levelVariable == trimmed) return;
    m_levelVariable = trimmed;
    m_settings.setValue(kLevelVariable, trimmed);
    accepted(QStringLiteral("levelVariable"));
    emit levelVariableChanged();
}

void SettingsManager::setVolumeVariable(const QString &variable)
{
    const QString trimmed = variable.trimmed();
    if (trimmed.isEmpty()) {
        rejected(QStringLiteral("volumeVariable"), QStringLiteral("cannot be empty"));
        return;
    }
    if (m_volumeVariable == trimmed) return;
    m_volumeVariable = trimmed;
    m_settings.setValue(kVolumeVariable, trimmed);
    accepted(QStringLiteral("volumeVariable"));
    emit volumeVariableChanged();
}

void SettingsManager::setDistanceVariable(const QString &variable)
{
    const QString trimmed = variable.trimmed();
    if (trimmed.isEmpty()) {
        rejected(QStringLiteral("distanceVariable"), QStringLiteral("cannot be empty"));
        return;
    }
    if (m_distanceVariable == trimmed) return;
    m_distanceVariable = trimmed;
    m_settings.setValue(kDistanceVariable, trimmed);
    accepted(QStringLiteral("distanceVariable"));
    emit distanceVariableChanged();
}

void SettingsManager::setTemperatureVariable(const QString &variable)
{
    const QString trimmed = variable.trimmed();
    if (m_temperatureVariable == trimmed) return;
    m_temperatureVariable = trimmed;
    m_settings.setValue(kTemperatureVariable, trimmed);
    accepted(QStringLiteral("temperatureVariable"));
    emit temperatureVariableChanged();
}


void SettingsManager::setPressureVariable(const QString &variable)
{
    const QString trimmed = variable.trimmed();
    if (trimmed.isEmpty()) {
        rejected(QStringLiteral("pressureVariable"), QStringLiteral("cannot be empty"));
        return;
    }
    if (m_pressureVariable == trimmed) return;
    m_pressureVariable = trimmed;
    m_settings.setValue(kPressureVariable, trimmed);
    accepted(QStringLiteral("pressureVariable"));
    emit pressureVariableChanged();
}

void SettingsManager::setStatusVariable(const QString &variable)
{
    const QString trimmed = variable.trimmed();
    if (trimmed.isEmpty()) {
        rejected(QStringLiteral("statusVariable"), QStringLiteral("cannot be empty"));
        return;
    }
    if (m_statusVariable == trimmed) return;
    m_statusVariable = trimmed;
    m_settings.setValue(kStatusVariable, trimmed);
    accepted(QStringLiteral("statusVariable"));
    emit statusVariableChanged();
}

void SettingsManager::setReadingType(const QString &type)
{
    const QString trimmed = type.trimmed();
    if (!isValidReadingType(trimmed)) {
        rejected(QStringLiteral("readingType"), QStringLiteral("must be Level, Volume, or Distance"));
        return;
    }
    if (m_readingType == trimmed) return;
    m_readingType = trimmed;
    m_settings.setValue(kReadingType, trimmed);
    accepted(QStringLiteral("readingType"));
    emit readingTypeChanged();
}

void SettingsManager::setSensorMountingOffset(double offset)
{
    if (!std::isfinite(offset)) {
        rejected(QStringLiteral("sensorMountingOffset"), QStringLiteral("must be a finite number"));
        return;
    }
    if (qFuzzyCompare(m_sensorMountingOffset + 1.0, offset + 1.0)) return;
    m_sensorMountingOffset = offset;
    m_settings.setValue(kSensorMountingOffset, offset);
    accepted(QStringLiteral("sensorMountingOffset"));
    emit sensorMountingOffsetChanged();
}

void SettingsManager::setRefreshIntervalMs(int ms)
{
    if (ms < 1000) {
        rejected(QStringLiteral("refreshIntervalMs"), QStringLiteral("must be >= 1000"));
        return;
    }
    if (m_refreshIntervalMs == ms) return;
    m_refreshIntervalMs = ms;
    m_settings.setValue(kRefreshIntervalMs, ms);
    accepted(QStringLiteral("refreshIntervalMs"));
    emit refreshIntervalMsChanged();
}

void SettingsManager::setTimeoutMs(int ms)
{
    if (ms < 1000) {
        rejected(QStringLiteral("timeoutMs"), QStringLiteral("must be >= 1000"));
        return;
    }
    if (m_timeoutMs == ms) return;
    m_timeoutMs = ms;
    m_settings.setValue(kTimeoutMs, ms);
    accepted(QStringLiteral("timeoutMs"));
    emit timeoutMsChanged();
}

// --- Tank group --------------------------------------------------------

void SettingsManager::setTankName(const QString &name)
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        rejected(QStringLiteral("tankName"), QStringLiteral("cannot be empty"));
        return;
    }
    if (m_tankName == trimmed) return;
    m_tankName = trimmed;
    m_settings.setValue(kTankName, trimmed);
    accepted(QStringLiteral("tankName"));
    emit tankNameChanged();
}

void SettingsManager::setTankRadius(double meters)
{
    if (!std::isfinite(meters)) {
        rejected(QStringLiteral("tankRadius"), QStringLiteral("must be a finite number"));
        return;
    }
    if (meters <= 0.0) {
        rejected(QStringLiteral("tankRadius"), QStringLiteral("must be positive"));
        return;
    }
    if (qFuzzyCompare(m_tankRadius + 1.0, meters + 1.0)) return;
    m_tankRadius = meters;
    m_settings.setValue(kTankRadius, meters);
    accepted(QStringLiteral("tankRadius"));
    emit tankRadiusChanged();
}

void SettingsManager::setTankHeight(double meters)
{
    if (!std::isfinite(meters)) {
        rejected(QStringLiteral("tankHeight"), QStringLiteral("must be a finite number"));
        return;
    }
    if (meters <= 0.0) {
        rejected(QStringLiteral("tankHeight"), QStringLiteral("must be positive"));
        return;
    }
    if (qFuzzyCompare(m_tankHeight + 1.0, meters + 1.0)) return;
    m_tankHeight = meters;
    m_settings.setValue(kTankHeight, meters);
    accepted(QStringLiteral("tankHeight"));
    emit tankHeightChanged();
}

// --- Logging group -----------------------------------------------------

void SettingsManager::setLogLevel(const QString &level)
{
    const QString trimmed = level.trimmed();
    if (!isValidLogLevel(trimmed)) {
        rejected(QStringLiteral("logLevel"), QStringLiteral("must be a recognized log level"));
        return;
    }
    if (m_logLevel == trimmed) return;
    m_logLevel = trimmed;
    m_settings.setValue(kLogLevel, trimmed);
    accepted(QStringLiteral("logLevel"));
    emit logLevelChanged();
}

void SettingsManager::setLogRetentionDays(int days)
{
    if (days < 1 || days > 90) {
        rejected(QStringLiteral("logRetentionDays"), QStringLiteral("must be between 1 and 90"));
        return;
    }
    if (m_logRetentionDays == days) return;
    m_logRetentionDays = days;
    m_settings.setValue(kLogRetentionDays, days);
    accepted(QStringLiteral("logRetentionDays"));
    emit logRetentionDaysChanged();
}

// --- Theme group -----------------------------------------------------------

void SettingsManager::setTheme(const QString &theme)
{
    const QString trimmed = theme.trimmed();
    if (!isValidTheme(trimmed)) {
        rejected(QStringLiteral("theme"), QStringLiteral("must be Light or Dark"));
        return;
    }
    if (m_theme == trimmed) return;
    m_theme = trimmed;
    m_settings.setValue(kTheme, trimmed);
    accepted(QStringLiteral("theme"));
    emit themeChanged();
}

} // namespace Panorama