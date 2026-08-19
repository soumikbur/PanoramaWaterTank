// ApiClient.h
#ifndef PANORAMA_APICLIENT_H
#define PANORAMA_APICLIENT_H

#include <QObject>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QUrl>
#include <QString>
#include <QJsonArray>

namespace Panorama {

class ApiClient : public QObject
{
    Q_OBJECT

public:
    explicit ApiClient(QObject *parent = nullptr);
    ~ApiClient() override = default;

    void setApiConfiguration(const QString &baseUrl,
                             const QString &token,
                             const QString &deviceLabel,
                             int pollingIntervalMs,
                             int transferTimeoutMs,
                             const QString &levelVar,
                             const QString &tempVar,
                             const QString &pressureVar,
                             const QString &statusVar);

    int historyRangeHours() const { return m_historyRangeHours; }

public slots:
    void startPolling();
    void stopPolling();
    void setHistoryRangeHours(int hours);
    void fetchHistoricalData();

signals:
    void readingReceived(double waterLevel, double temperature, double pressure, QString sensorStatus);
    void historicalDataReceived(const QVariantList &history);
    void networkError(QString error);
    void connectionStateChanged(bool connected);

private slots:
    void fetchLatestData();
    void onReplyFinished(QNetworkReply *reply);

private:
    [[nodiscard]] QNetworkRequest buildRequest() const;
    [[nodiscard]] QNetworkRequest buildHistoricalRequest() const;
    void handleNetworkError(QNetworkReply *reply);
    void parseReply(const QByteArray &data);
    void parseHistoricalReply(QNetworkReply *reply);
    void extractVariables(const QJsonDocument &doc);

    QNetworkAccessManager m_networkManager;
    QTimer m_pollTimer;
    QTimer m_historyTimer;
    int m_historyRangeHours{12};

    QString m_apiBaseUrl;
    QString m_authToken;
    QString m_deviceLabel;

    int m_pollingIntervalMs{15000};
    int m_transferTimeoutMs{10000};

    QString m_levelVar;
    QString m_tempVar;
    QString m_pressureVar;
    QString m_statusVar;

    bool m_isConnected{false};
    bool m_requestInProgress{false};

           // Cached last valid values for partial/missing variable tolerance
    double m_lastValidWaterLevel{0.0};
    double m_lastValidTemperature{25.0};
    double m_lastValidPressure{1.0};
    QString m_lastValidSensorStatus{QStringLiteral("Healthy")};
    bool m_hasReceivedFirstValidReading{false};
};

} // namespace Panorama

#endif // PANORAMA_APICLIENT_H