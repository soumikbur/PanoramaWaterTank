// TankRepository.cpp
#include "TankRepository.h"
#include "TankModel.h"
#include "Logger.h"
#include <cmath>

namespace Panorama {

namespace {
constexpr double MIN_WATER_LEVEL_METERS = 0.0;
constexpr double MAX_REASONABLE_WATER_LEVEL_METERS = 100.0;

constexpr double MIN_TEMPERATURE_C = -40.0;
constexpr double MAX_TEMPERATURE_C = 100.0;

constexpr double MIN_PRESSURE = 0.0;
}

TankRepository::TankRepository(Logger &logger, QObject *parent)
    : QObject(parent)
      , m_logger(logger)
{
}

void TankRepository::setTankModel(TankModel *model)
{
    m_tankModel = model;
}

void TankRepository::processReading(double waterLevel, double temperature, double pressure, const QString &sensorStatus)
{
    bool acceptedAny = false;

    if (validateWaterLevel(waterLevel)) {
        m_cachedWaterLevel = waterLevel;
        acceptedAny = true;
    } else {
        m_logger.logError(Logger::LogLevel::Warning, QStringLiteral("TankRepository: Rejected invalid water level: %1").arg(waterLevel));
    }

    if (validateTemperature(temperature)) {
        m_cachedTemperature = temperature;
        acceptedAny = true;
    } else {
        m_logger.logError(Logger::LogLevel::Warning, QStringLiteral("TankRepository: Rejected invalid temperature: %1").arg(temperature));
    }

    if (validatePressure(pressure)) {
        m_cachedPressure = pressure;
        m_hasPressure = true;
        acceptedAny = true;
    } else {
        m_logger.logError(Logger::LogLevel::Warning, QStringLiteral("TankRepository: Rejected invalid pressure: %1").arg(pressure));
    }

    if (sensorStatus == QStringLiteral("Healthy") ||
        sensorStatus == QStringLiteral("Fault") ||
        sensorStatus == QStringLiteral("Unknown")) {
        m_cachedSensorStatus = sensorStatus;
        acceptedAny = true;
    } else {
        m_logger.logError(Logger::LogLevel::Warning, QStringLiteral("TankRepository: Rejected invalid sensor status: %1").arg(sensorStatus));
    }

    if (acceptedAny && m_tankModel) {
        // Convert water level from cm to meters if reported in cm (e.g. 28.0 cm -> 0.28 m)
        double waterLevelMeters = m_cachedWaterLevel;
        if (waterLevelMeters > 2.0) {
            waterLevelMeters /= 100.0;
        }

        m_tankModel->applyReading(
            true,
            waterLevelMeters,
            false,
            0.0,
            m_cachedTemperature,
            m_hasPressure,
            m_cachedPressure,
            m_cachedSensorStatus
            );

        m_logger.logApplication(
            Logger::LogLevel::Debug,
            QStringLiteral("TankRepository: Water Height = %1 cm, Tank Height = %2 cm, Fill % = %3%, Volume = %4 L, Capacity = %5 L")
                .arg(m_tankModel->waterHeightCm(), 0, 'f', 2)
                .arg(m_tankModel->tankHeightCm(), 0, 'f', 2)
                .arg(m_tankModel->fillPercentage(), 0, 'f', 2)
                .arg(m_tankModel->waterVolumeLiters(), 0, 'f', 2)
                .arg(m_tankModel->capacityLiters(), 0, 'f', 2)
            );
    }

}


bool TankRepository::validateWaterLevel(double value) const
{
    if (std::isnan(value) || std::isinf(value)) {
        return false;
    }
    return (value >= MIN_WATER_LEVEL_METERS && value <= MAX_REASONABLE_WATER_LEVEL_METERS);
}

bool TankRepository::validateTemperature(double value) const
{
    if (std::isnan(value) || std::isinf(value)) {
        return false;
    }
    return (value >= MIN_TEMPERATURE_C && value <= MAX_TEMPERATURE_C);
}

bool TankRepository::validatePressure(double value) const
{
    if (std::isnan(value) || std::isinf(value)) {
        return false;
    }
    return (value >= MIN_PRESSURE);
}

} // namespace Panorama