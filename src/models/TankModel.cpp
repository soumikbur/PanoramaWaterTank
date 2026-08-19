#include "TankModel.h"

#include "VolumeCalculator.h"
#include "HistoryExporter.h"

#include <QDateTime>
#include <algorithm>
#include <cmath>

namespace Panorama {

TankModel::TankModel(QObject *parent)
    : QObject(parent)
{
    recalculateGeometry();
}

void TankModel::setTankName(const QString &name)
{
    if (m_tankName == name) {
        return;
    }
    m_tankName = name;
    emit tankNameChanged();
}

void TankModel::setDeviceLabel(const QString &label)
{
    if (m_deviceLabel == label) {
        return;
    }
    m_deviceLabel = label;
    emit deviceLabelChanged();
}

void TankModel::setTankRadius(double radiusMeters)
{
    if (qFuzzyCompare(m_tankRadius + 1.0, radiusMeters + 1.0)) {
        return;
    }
    m_tankRadius = radiusMeters;
    emit tankRadiusChanged();
    recalculateGeometry();
}

void TankModel::setTankHeight(double heightMeters)
{
    if (qFuzzyCompare(m_tankHeight + 1.0, heightMeters + 1.0)) {
        return;
    }
    m_tankHeight = heightMeters;
    emit tankHeightChanged();
    recalculateGeometry();
}

void TankModel::setConnectionState(const QString &state)
{
    if (m_connectionState == state) {
        return;
    }
    m_connectionState = state;
    emit connectionStateChanged();
}

void TankModel::recalculateGeometry()
{
    m_tankArea = VolumeCalculator::calculateArea(m_tankRadius);
    emit tankAreaChanged();

    m_maximumVolume = VolumeCalculator::calculateVolume(m_tankArea, m_tankHeight);
    emit maximumVolumeChanged();

    // Re-derive the current reading against the new geometry so the
    // dashboard never shows a percentage computed against stale geometry
    // (docs/04-engineering-model-workflow.md, Section 8's "Geometry
    // changed" sequence).
    recalculateFromHeight(m_currentHeight);
}

void TankModel::recalculateFromHeight(double heightMeters)
{
    m_currentHeight = std::max(0.0, heightMeters);
    m_currentVolume = VolumeCalculator::calculateVolume(m_tankArea, m_currentHeight);

    m_fillPercentageRaw = VolumeCalculator::calculateFillPercentageRaw(m_currentVolume, m_maximumVolume);
    m_fillPercentage = VolumeCalculator::clampPercentageForDisplay(m_fillPercentageRaw);
    m_emptyPercentage = 100.0 - m_fillPercentage;
    m_remainingVolume = VolumeCalculator::calculateRemainingVolume(m_maximumVolume, m_currentVolume);

    emit currentHeightChanged();
    emit currentVolumeChanged();
    emit fillPercentageChanged();
    emit emptyPercentageChanged();
    emit remainingVolumeChanged();

    deriveCapacityRank();
    deriveAlarmLevel();
}

void TankModel::recalculateFromVolume(double volumeLiters)
{
    m_currentVolume = std::max(0.0, volumeLiters);
    m_currentHeight = VolumeCalculator::calculateHeight(m_currentVolume, m_tankArea);

    m_fillPercentageRaw = VolumeCalculator::calculateFillPercentageRaw(m_currentVolume, m_maximumVolume);
    m_fillPercentage = VolumeCalculator::clampPercentageForDisplay(m_fillPercentageRaw);
    m_emptyPercentage = 100.0 - m_fillPercentage;
    m_remainingVolume = VolumeCalculator::calculateRemainingVolume(m_maximumVolume, m_currentVolume);

    emit currentHeightChanged();
    emit currentVolumeChanged();
    emit fillPercentageChanged();
    emit emptyPercentageChanged();
    emit remainingVolumeChanged();

    deriveCapacityRank();
    deriveAlarmLevel();
}

void TankModel::deriveCapacityRank()
{
    // Purely descriptive quartile (docs/03-ui-ux-design-system.md,
    // Section 8) - never subject to hysteresis, never operationally
    // actionable on its own. Judged against the clamped display
    // percentage, unlike alarmLevel's Overflow case below.
    QString newRank;
    if (m_fillPercentage < 25.0) {
        newRank = QStringLiteral("Low");
    } else if (m_fillPercentage < 50.0) {
        newRank = QStringLiteral("Moderate");
    } else if (m_fillPercentage < 75.0) {
        newRank = QStringLiteral("Good");
    } else {
        newRank = QStringLiteral("High");
    }

    if (m_capacityRank == newRank) {
        return;
    }
    m_capacityRank = newRank;
    emit capacityRankChanged();
}

void TankModel::deriveAlarmLevel()
{
    // docs/04-engineering-model-workflow.md, Section 10: Overflow is
    // judged against the RAW (unclamped) percentage specifically, never
    // the clamped display value - a raw reading of 102% is a real
    // overflow signal that clamping would otherwise hide.
    QString newLevel;
    if (m_fillPercentageRaw >= 100.0 || (m_tankHeight > 0.0 && m_currentHeight >= m_tankHeight)) {
        newLevel = QStringLiteral("Overflow");
    } else if (m_fillPercentage < 10.0) {
        newLevel = QStringLiteral("Critical");
    } else if (m_fillPercentage < 20.0) {
        newLevel = QStringLiteral("Warning");
    } else {
        newLevel = QStringLiteral("Normal");
    }

    if (m_alarmLevel == newLevel) {
        return;
    }
    m_alarmLevel = newLevel;
    emit alarmLevelChanged();
}

void TankModel::touchLastUpdated()
{
    // 12-hour clock with AM/PM (e.g. "10:42:11 AM"), matching the
    // dashboard redesign spec's examples. Same property, same type -
    // only the display format changed.
    m_lastUpdated = QDateTime::currentDateTime().toString(QStringLiteral("h:mm:ss AP"));
    emit lastUpdatedChanged();
}

void TankModel::applyReading(bool hasHeight, double heightMeters, bool hasVolume, double volumeLiters,
                              double temperatureC, bool hasPressureVal, double pressureVal, const QString &sensorStatus)
{
    if (hasHeight) {
        recalculateFromHeight(heightMeters);
    } else if (hasVolume) {
        recalculateFromVolume(volumeLiters);
    }

    if (!m_hasTemperature || !qFuzzyCompare(m_temperature + 1.0, temperatureC + 1.0)) {
        m_temperature = temperatureC;
        emit temperatureChanged();
    }
    if (!m_hasTemperature) {
        m_hasTemperature = true;
        emit hasTemperatureChanged();
    }

    if (hasPressureVal) {
        if (!m_hasPressure || !qFuzzyCompare(m_pressure + 1.0, pressureVal + 1.0)) {
            m_pressure = pressureVal;
            emit pressureChanged();
        }
        if (!m_hasPressure) {
            m_hasPressure = true;
            emit hasPressureChanged();
        }
    }

    if (!sensorStatus.isEmpty() && sensorStatus != m_sensorStatus) {
        m_sensorStatus = sensorStatus;
        emit sensorStatusChanged();
    }

    touchLastUpdated();
}


void TankModel::setTrendHistory(const QVariantList &history)
{
    m_trendHistory = history;
    emit trendHistoryChanged();
}

void TankModel::setHistoryRangeHours(int hours)
{
    if (hours <= 0) hours = 12;
    if (m_historyRangeHours != hours) {
        m_historyRangeHours = hours;
        emit historyRangeHoursChanged(hours);
    }
}

void TankModel::refreshTrend()
{
    emit trendRefreshRequested();
}

QVariantMap TankModel::exportHistoryCsv(const QString &filePath, int rangeHours)
{
    const int effectiveHours = (rangeHours > 0) ? rangeHours : m_historyRangeHours;
    HistoryExporter::ExportResult result = HistoryExporter::exportToFile(
        filePath,
        m_trendHistory,
        effectiveHours,
        tankHeightCm(),
        capacityLiters(),
        100 // Target ~100 representative points
    );

    QVariantMap map;
    map[QStringLiteral("success")] = result.success;
    map[QStringLiteral("count")] = result.pointCount;
    map[QStringLiteral("filePath")] = result.filePath;
    map[QStringLiteral("fileName")] = result.fileName;
    map[QStringLiteral("message")] = result.success
        ? QStringLiteral("Successfully exported %1 historical data points to %2").arg(QString::number(result.pointCount), result.fileName)
        : result.errorMessage;
    return map;
}

QString TankModel::defaultCsvFilename(int rangeHours) const
{
    const int effectiveHours = (rangeHours > 0) ? rangeHours : m_historyRangeHours;
    return HistoryExporter::defaultFilename(effectiveHours);
}

bool TankModel::hasHistoricalData(int rangeHours) const
{
    const int effectiveHours = (rangeHours > 0) ? rangeHours : m_historyRangeHours;
    const auto points = HistoryExporter::processHistoricalData(
        m_trendHistory,
        effectiveHours,
        tankHeightCm(),
        capacityLiters(),
        100
    );
    return !points.isEmpty();
}

QString TankModel::getCsvPreview(int rangeHours) const
{
    const int effectiveHours = (rangeHours > 0) ? rangeHours : m_historyRangeHours;
    const auto points = HistoryExporter::processHistoricalData(
        m_trendHistory,
        effectiveHours,
        tankHeightCm(),
        capacityLiters(),
        100
    );
    return HistoryExporter::formatCsv(points);
}

} // namespace Panorama


