#pragma once

#include <QObject>
#include <QString>
#include <qqml.h>

namespace Panorama {

/*!
 * \brief QML-facing single source of truth for tank geometry and current
 *        derived state (docs/05-implementation-blueprint.md, Section 1
 *        and Section 2).
 *
 * Exposed as a QML singleton. Fully implemented here (ahead of its
 * nominal Milestone 4 slot) because the dashboard redesign (docs/07)
 * requires real, bindable properties rather than hardcoded values - this
 * is a direct transcription of docs/05, Section 2's already-approved
 * property catalog, not new design.
 *
 * Every property is technically settable at the C++ level (needed for a
 * future SettingsManager/TankRepository to push values in) but is
 * read-only from QML by convention (docs/05, Section 2's resolution): a
 * future Settings page writes through SettingsManager, never directly
 * through TankModel.
 *
 * ApiClient/TankRepository are not wired to this class yet (that is
 * Milestone 5's networking work), so connectionState/fillPercentage/etc.
 * currently hold their documented defaults rather than live sensor
 * values - the bindings are real, the data just has nothing feeding it
 * yet.
 */
class TankModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

           // Geometry
    Q_PROPERTY(QString tankName READ tankName NOTIFY tankNameChanged)
    Q_PROPERTY(QString deviceLabel READ deviceLabel NOTIFY deviceLabelChanged)
    Q_PROPERTY(double tankRadius READ tankRadius NOTIFY tankRadiusChanged)
    Q_PROPERTY(double tankHeight READ tankHeight NOTIFY tankHeightChanged)

           // Engineering
    Q_PROPERTY(double tankArea READ tankArea NOTIFY tankAreaChanged)
    Q_PROPERTY(double maximumVolume READ maximumVolume NOTIFY maximumVolumeChanged)
    Q_PROPERTY(double currentHeight READ currentHeight NOTIFY currentHeightChanged)
    Q_PROPERTY(double currentVolume READ currentVolume NOTIFY currentVolumeChanged)
    Q_PROPERTY(double waterHeightCm READ waterHeightCm NOTIFY currentHeightChanged)
    Q_PROPERTY(double tankHeightCm READ tankHeightCm NOTIFY tankHeightChanged)
    Q_PROPERTY(double waterVolumeLiters READ waterVolumeLiters NOTIFY currentVolumeChanged)
    Q_PROPERTY(double capacityLiters READ capacityLiters NOTIFY maximumVolumeChanged)
    Q_PROPERTY(double fillPercentage READ fillPercentage NOTIFY fillPercentageChanged)
    Q_PROPERTY(double rawFillPercentage READ rawFillPercentage NOTIFY fillPercentageChanged)
    Q_PROPERTY(double tankRadiusCm READ tankRadiusCm NOTIFY tankRadiusChanged)
    Q_PROPERTY(double emptyPercentage READ emptyPercentage NOTIFY emptyPercentageChanged)
    Q_PROPERTY(double remainingVolume READ remainingVolume NOTIFY remainingVolumeChanged)
    Q_PROPERTY(QString capacityRank READ capacityRank NOTIFY capacityRankChanged)

           // Status & alarm
    Q_PROPERTY(QString alarmLevel READ alarmLevel NOTIFY alarmLevelChanged)
    Q_PROPERTY(QString sensorStatus READ sensorStatus NOTIFY sensorStatusChanged)

           // API-sourced
    Q_PROPERTY(double temperature READ temperature NOTIFY temperatureChanged)
    Q_PROPERTY(bool hasTemperature READ hasTemperature NOTIFY hasTemperatureChanged)
    Q_PROPERTY(double pressure READ pressure NOTIFY pressureChanged)
    Q_PROPERTY(bool hasPressure READ hasPressure NOTIFY hasPressureChanged)

           // Connection
    Q_PROPERTY(QString connectionState READ connectionState NOTIFY connectionStateChanged)

           // Display / time / trend
    Q_PROPERTY(QString lastUpdated READ lastUpdated NOTIFY lastUpdatedChanged)
    Q_PROPERTY(QVariantList trendHistory READ trendHistory NOTIFY trendHistoryChanged)
    Q_PROPERTY(int historyRangeHours READ historyRangeHours WRITE setHistoryRangeHours NOTIFY historyRangeHoursChanged)

public:
    explicit TankModel(QObject *parent = nullptr);

    QString tankName() const { return m_tankName; }
    QString deviceLabel() const { return m_deviceLabel; }
    double tankRadius() const { return m_tankRadius; }
    double tankRadiusCm() const { return m_tankRadius * 100.0; }
    double tankHeight() const { return m_tankHeight; }

    double tankArea() const { return m_tankArea; }
    double maximumVolume() const { return m_maximumVolume; }
    double currentHeight() const { return m_currentHeight; }
    double currentVolume() const { return m_currentVolume; }
    double waterHeightCm() const { return m_currentHeight * 100.0; }
    double tankHeightCm() const { return m_tankHeight * 100.0; }
    double waterVolumeLiters() const { return m_currentVolume; }
    double capacityLiters() const { return m_maximumVolume; }
    double fillPercentage() const { return m_fillPercentage; }
    double rawFillPercentage() const { return m_fillPercentageRaw; }
    double emptyPercentage() const { return m_emptyPercentage; }
    double remainingVolume() const { return m_remainingVolume; }
    QString capacityRank() const { return m_capacityRank; }

    QString alarmLevel() const { return m_alarmLevel; }
    QString sensorStatus() const { return m_sensorStatus; }

    double temperature() const { return m_temperature; }
    bool hasTemperature() const { return m_hasTemperature; }
    double pressure() const { return m_pressure; }
    bool hasPressure() const { return m_hasPressure; }

    QString connectionState() const { return m_connectionState; }

    QString lastUpdated() const { return m_lastUpdated; }
    QVariantList trendHistory() const { return m_trendHistory; }
    int historyRangeHours() const { return m_historyRangeHours; }

    void setTankName(const QString &name);
    void setDeviceLabel(const QString &label);
    void setTankRadius(double radiusMeters);
    void setTankHeight(double heightMeters);
    void setConnectionState(const QString &state);
    void setTrendHistory(const QVariantList &history);
    void setHistoryRangeHours(int hours);

    Q_INVOKABLE void refreshTrend();
    Q_INVOKABLE QVariantMap exportHistoryCsv(const QString &filePath, int rangeHours = 0);
    Q_INVOKABLE QString defaultCsvFilename(int rangeHours = 0) const;
    Q_INVOKABLE bool hasHistoricalData(int rangeHours = 0) const;
    Q_INVOKABLE QString getCsvPreview(int rangeHours = 0) const;

public slots:
    //! Mirrors the ValidatedReading contract
    //! (docs/04-engineering-model-workflow.md, Section 12).
    void applyReading(bool hasHeight, double heightMeters, bool hasVolume, double volumeLiters,
                      double temperatureC, bool hasPressureVal, double pressureVal, const QString &sensorStatus);

signals:
    void tankNameChanged();
    void deviceLabelChanged();
    void tankRadiusChanged();
    void tankHeightChanged();
    void tankAreaChanged();
    void maximumVolumeChanged();
    void currentHeightChanged();
    void currentVolumeChanged();
    void fillPercentageChanged();
    void emptyPercentageChanged();
    void remainingVolumeChanged();
    void capacityRankChanged();
    void alarmLevelChanged();
    void sensorStatusChanged();
    void temperatureChanged();
    void hasTemperatureChanged();
    void pressureChanged();
    void hasPressureChanged();
    void connectionStateChanged();
    void lastUpdatedChanged();
    void trendHistoryChanged();
    void historyRangeHoursChanged(int hours);
    void trendRefreshRequested();


private:
    void recalculateGeometry();
    void recalculateFromHeight(double heightMeters);
    void recalculateFromVolume(double volumeLiters);
    void deriveCapacityRank();
    void deriveAlarmLevel();
    void touchLastUpdated();

    QString m_tankName = QStringLiteral("Panorama Water Tank");
    QString m_deviceLabel = QStringLiteral("esp32s3");
    double m_tankRadius = 0.11283791670955126;
    double m_tankHeight = 0.50; // Physical tank height = 50 cm (0.50 m)


    double m_tankArea = 0.0;
    double m_maximumVolume = 0.0;
    double m_currentHeight = 0.0;
    double m_currentVolume = 0.0;
    double m_fillPercentage = 0.0;
    double m_fillPercentageRaw = 0.0;
    double m_emptyPercentage = 100.0;
    double m_remainingVolume = 0.0;
    QString m_capacityRank = QStringLiteral("Low");

    QString m_alarmLevel;
    QString m_sensorStatus;

    double m_temperature = 0.0;
    bool m_hasTemperature = false;
    double m_pressure = 0.0;
    bool m_hasPressure = false;

    QString m_connectionState = QStringLiteral("Connecting");

    QString m_lastUpdated = QStringLiteral("--");
    QVariantList m_trendHistory;
    int m_historyRangeHours{12};
};


} // namespace Panorama


