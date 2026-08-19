#include "ApiClient.h"
#include <QDebug>
#include <QDateTime>
#include <QTimeZone>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QVariant>
#include <algorithm>
#include <cmath>

namespace Panorama {

ApiClient::ApiClient(QObject *parent)
    : QObject(parent)
{
    connect(&m_pollTimer, &QTimer::timeout, this, &ApiClient::fetchLatestData);
    connect(&m_historyTimer, &QTimer::timeout, this, &ApiClient::fetchHistoricalData);
    connect(&m_networkManager, &QNetworkAccessManager::finished, this, &ApiClient::onReplyFinished);
}

void ApiClient::setApiConfiguration(const QString &baseUrl,
                                    const QString &token,
                                    const QString &deviceLabel,
                                    int pollingIntervalMs,
                                    int transferTimeoutMs,
                                    const QString &levelVar,
                                    const QString &tempVar,
                                    const QString &pressureVar,
                                    const QString &statusVar)
{
    m_apiBaseUrl = baseUrl.trimmed();
    m_authToken = token.trimmed();
    m_deviceLabel = deviceLabel.trimmed();

    m_pollingIntervalMs = (pollingIntervalMs > 0) ? pollingIntervalMs : 15000;
    m_transferTimeoutMs = (transferTimeoutMs > 0) ? transferTimeoutMs : 10000;

    m_levelVar = levelVar.trimmed();
    m_tempVar = tempVar.trimmed();
    m_pressureVar = pressureVar.trimmed();
    m_statusVar = statusVar.trimmed();

    if (m_pollTimer.isActive() && m_pollTimer.interval() != m_pollingIntervalMs) {
        m_pollTimer.setInterval(m_pollingIntervalMs);
    }
}

void ApiClient::setHistoryRangeHours(int hours)
{
    if (hours <= 0) hours = 12;
    if (m_historyRangeHours != hours) {
        m_historyRangeHours = hours;
        qDebug() << "[Trend] History range changed to" << hours << "hours. Fetching new historical data.";
        fetchHistoricalData();
    }
}

void ApiClient::startPolling()
{
    if (!m_pollTimer.isActive()) {
        m_pollTimer.start(m_pollingIntervalMs);
        fetchLatestData();
    }
    if (!m_historyTimer.isActive()) {
        m_historyTimer.start(60000); // 60s slow background history refresh
        fetchHistoricalData();
    }
}

void ApiClient::stopPolling()
{
    if (m_pollTimer.isActive()) {
        m_pollTimer.stop();
    }
    if (m_historyTimer.isActive()) {
        m_historyTimer.stop();
    }
}

void ApiClient::fetchLatestData()
{
    if (m_authToken.isEmpty() || m_apiBaseUrl.isEmpty() || m_deviceLabel.isEmpty()) {
        if (m_isConnected) {
            m_isConnected = false;
            emit connectionStateChanged(false);
        }
        emit networkError(QStringLiteral("Missing API configuration (Base URL, Token, or Device Label)."));
        return;
    }

    if (!m_requestInProgress) {
        m_requestInProgress = true;
        QNetworkRequest request = buildRequest();
        m_networkManager.get(request);
    }
}

void ApiClient::fetchHistoricalData()
{
    if (m_authToken.isEmpty() || m_apiBaseUrl.isEmpty() || m_deviceLabel.isEmpty()) {
        return;
    }

    QNetworkRequest request = buildHistoricalRequest();
    m_networkManager.get(request);
}

QNetworkRequest ApiClient::buildRequest() const
{
    QString urlStr = m_apiBaseUrl;
    if (!urlStr.endsWith(QLatin1Char('/'))) {
        urlStr += QLatin1Char('/');
    }

    QString deviceKey = m_deviceLabel.trimmed();
    if (!deviceKey.startsWith(QLatin1Char('~'))) {
        deviceKey = QLatin1Char('~') + deviceKey;
    }

    // Ubidots v2.0 endpoint for fetching all latest variable values by device API label:
    // /api/v2.0/devices/~{deviceLabel}/_/values/last
    urlStr += QStringLiteral("api/v2.0/devices/") + deviceKey + QStringLiteral("/_/values/last");

    QNetworkRequest request((QUrl(urlStr)));
    request.setRawHeader("X-Auth-Token", m_authToken.toUtf8());
    request.setRawHeader("Content-Type", "application/json");
    request.setTransferTimeout(m_transferTimeoutMs);
    return request;
}

QNetworkRequest ApiClient::buildHistoricalRequest() const
{
    QString urlStr = m_apiBaseUrl;
    if (!urlStr.endsWith(QLatin1Char('/'))) {
        urlStr += QLatin1Char('/');
    }

    const QString levelVar = m_levelVar.isEmpty() ? QStringLiteral("waterlevel") : m_levelVar.trimmed();

    const qint64 endMs = QDateTime::currentMSecsSinceEpoch();
    const qint64 durationMs = static_cast<qint64>(m_historyRangeHours) * 60LL * 60LL * 1000LL;
    const qint64 startMs = endMs - durationMs;

    qDebug() << "[Trend] Building API historical request: Range Hours =" << m_historyRangeHours
             << "Duration Ms =" << durationMs << "startMs =" << startMs << "endMs =" << endMs;

    // Ubidots v1.6 endpoint for fetching historical waterlevel values over dynamic range:
    // /api/v1.6/devices/{deviceLabel}/{levelVar}/values?start={startMs}&end={endMs}&page_size=500
    urlStr += QStringLiteral("api/v1.6/devices/") + m_deviceLabel.trimmed() + QLatin1Char('/') + levelVar + QStringLiteral("/values?start=") + QString::number(startMs) + QStringLiteral("&end=") + QString::number(endMs) + QStringLiteral("&page_size=500");

    QNetworkRequest request((QUrl(urlStr)));
    request.setRawHeader("X-Auth-Token", m_authToken.toUtf8());
    request.setRawHeader("Content-Type", "application/json");
    request.setTransferTimeout(m_transferTimeoutMs);
    return request;
}

void ApiClient::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    const QString urlStr = reply->request().url().toString();
    const bool isHistorical = urlStr.contains(QStringLiteral("/values")) && !urlStr.contains(QStringLiteral("/_/values/last"));

    if (isHistorical) {
        parseHistoricalReply(reply);
        return;
    }

    m_requestInProgress = false;

    if (reply->error() != QNetworkReply::NoError) {
        handleNetworkError(reply);
        return;
    }

    const QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (statusCode.isValid()) {
        const int status = statusCode.toInt();
        if (status != 200 && status != 201 && status != 204) {
            handleNetworkError(reply);
            return;
        }
    }

    if (!m_isConnected) {
        m_isConnected = true;
        emit connectionStateChanged(true);
    }

    parseReply(reply->readAll());
}

void ApiClient::parseHistoricalReply(QNetworkReply *reply)
{
    const QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    const int status = statusCode.isValid() ? statusCode.toInt() : 0;

    // Safe Diagnostic Logging (NEVER log authentication token or header)
    qDebug() << "======================================";
    qDebug() << "Historical request URL:" << reply->request().url().toString();
    qDebug() << "HTTP status           :" << status;

    if (reply->error() != QNetworkReply::NoError || (status != 200 && status != 201)) {
        qDebug() << "Historical request failed:" << reply->errorString();
        qDebug() << "======================================";
        return;
    }

    const QByteArray data = reply->readAll();
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        qDebug() << "Failed to parse historical JSON response.";
        qDebug() << "======================================";
        return;
    }

    QJsonObject rootObj = jsonDoc.object();
    QJsonArray resultsArray;
    if (rootObj.contains(QStringLiteral("results")) && rootObj.value(QStringLiteral("results")).isArray()) {
        resultsArray = rootObj.value(QStringLiteral("results")).toArray();
    }

    struct HistoricalPoint {
        qint64 timestampMs;
        double value;
    };
    QList<HistoricalPoint> points;

    for (const QJsonValue &item : resultsArray) {
        if (!item.isObject()) continue;
        QJsonObject o = item.toObject();
        if (!o.contains(QStringLiteral("value")) || !o.contains(QStringLiteral("timestamp"))) continue;

        QJsonValue valObj = o.value(QStringLiteral("value"));
        QJsonValue tsObj = o.value(QStringLiteral("timestamp"));

        bool okVal = false;
        double val = 0.0;
        if (valObj.isDouble()) {
            val = valObj.toDouble();
            okVal = true;
        } else if (valObj.isString()) {
            val = valObj.toString().toDouble(&okVal);
        }

        qint64 ts = 0;
        if (tsObj.isDouble()) {
            ts = static_cast<qint64>(tsObj.toDouble());
        } else if (tsObj.isString()) {
            const QString tsStr = tsObj.toString();
            bool okNum = false;
            ts = tsStr.toLongLong(&okNum);
            if (!okNum) {
                // Try parsing ISO-8601 date string (e.g. "2026-08-18T06:00:00Z")
                QDateTime dt = QDateTime::fromString(tsStr, Qt::ISODateWithMs);
                if (!dt.isValid()) {
                    dt = QDateTime::fromString(tsStr, Qt::ISODate);
                }
                if (dt.isValid()) {
                    ts = dt.toMSecsSinceEpoch();
                }
            }
        }

        // Canonical normalization: Unix seconds (< 10^11) to milliseconds
        if (ts > 0 && ts < 100000000000LL) {
            ts *= 1000LL;
        }

        if (!okVal || std::isnan(val) || std::isinf(val) || ts <= 0) {
            qWarning() << "[Chart] Invalid timestamp or value rejected:" << ts << val;
            continue;
        }

        points.append({ts, val});
    }

    // Sort points chronologically: oldest -> newest
    std::sort(points.begin(), points.end(), [](const HistoricalPoint &a, const HistoricalPoint &b) {
        return a.timestampMs < b.timestampMs;
    });

    qDebug() << "[Chart] Historical points received:" << points.size();
    if (!points.isEmpty()) {
        const QString firstUtc = QDateTime::fromMSecsSinceEpoch(points.first().timestampMs, QTimeZone::UTC).toString(Qt::ISODate);
        const QString lastUtc = QDateTime::fromMSecsSinceEpoch(points.last().timestampMs, QTimeZone::UTC).toString(Qt::ISODate);
        qDebug() << "[Chart] First:" << firstUtc << "UTC | timestampMs:" << points.first().timestampMs << "val:" << points.first().value;
        qDebug() << "[Chart] Last :" << lastUtc << "UTC | timestampMs:" << points.last().timestampMs << "val:" << points.last().value;
        qDebug() << "[Chart] Range:" << m_historyRangeHours << "hours | Timestamp unit: epoch milliseconds";
    }
    qDebug() << "======================================";

    QVariantList historyList;
    historyList.reserve(points.size());
    for (const HistoricalPoint &pt : points) {
        QVariantMap map;
        map[QStringLiteral("timestampMs")] = pt.timestampMs;
        map[QStringLiteral("timestamp")] = pt.timestampMs;
        map[QStringLiteral("value")] = pt.value;
        historyList.append(map);
    }

    emit historicalDataReceived(historyList);
}


void ApiClient::handleNetworkError(QNetworkReply *reply)
{
    if (m_isConnected) {
        m_isConnected = false;
        emit connectionStateChanged(false);
    }

    QString errorMessage = reply->errorString();
    const QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

    if (statusCode.isValid()) {
        const int status = statusCode.toInt();
        switch (status) {
            case 401: errorMessage = QStringLiteral("Authentication failed. Check the Ubidots token."); break;
            case 404: errorMessage = QStringLiteral("Device or endpoint not found. Verify the device API label and API URL."); break;
            case 429: errorMessage = QStringLiteral("Ubidots rate limit exceeded. Increase the refresh interval."); break;
            default: errorMessage = QStringLiteral("HTTP Error %1").arg(status); break;
        }
    } else {
        errorMessage = QStringLiteral("Ubidots server cannot be reached.");
    }

    emit networkError(errorMessage);
}

void ApiClient::parseReply(const QByteArray &data)
{
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        emit networkError(QStringLiteral("Failed to parse JSON response: ") + parseError.errorString());
        return;
    }

    if (!jsonDoc.isObject() && !jsonDoc.isArray()) {
        emit networkError(QStringLiteral("Invalid JSON structure: Expected an object or array."));
        return;
    }

    extractVariables(jsonDoc);
}


void ApiClient::extractVariables(const QJsonDocument &doc)
{
    bool foundNewData = false;

    const QString targetLevel = (m_levelVar.isEmpty() ? QStringLiteral("waterlevel") : m_levelVar).trimmed().toLower();
    const QString targetTemp = m_tempVar.trimmed().toLower();
    const QString targetPressure = (m_pressureVar.isEmpty() ? QStringLiteral("pressure") : m_pressureVar).trimmed().toLower();
    const QString targetStatus = (m_statusVar.isEmpty() ? QStringLiteral("sensorstatus") : m_statusVar).trimmed().toLower();

    auto processPair = [&](const QString &rawLabel, const QJsonValue &rawVal) {
        const QString label = rawLabel.trimmed().toLower();
        QJsonValue val = rawVal;

        if (val.isObject()) {
            QJsonObject o = val.toObject();
            if (o.contains(QStringLiteral("value"))) {
                val = o.value(QStringLiteral("value"));
            } else if (o.contains(QStringLiteral("last_value")) && o.value(QStringLiteral("last_value")).isObject()) {
                val = o.value(QStringLiteral("last_value")).toObject().value(QStringLiteral("value"));
            }
        }

        if (label == targetLevel) {
            if (val.isDouble()) {
                const double parsed = val.toDouble();
                if (!std::isnan(parsed) && !std::isinf(parsed) && parsed >= 0.0) {
                    m_lastValidWaterLevel = parsed;
                    foundNewData = true;
                }
            }
        } else if (!targetTemp.isEmpty() && label == targetTemp) {
            if (val.isDouble()) {
                const double parsed = val.toDouble();
                if (!std::isnan(parsed) && !std::isinf(parsed)) {
                    m_lastValidTemperature = parsed;
                    foundNewData = true;
                }
            }
        } else if (!targetPressure.isEmpty() && label == targetPressure) {
            bool ok = false;
            double parsed = 0.0;
            if (val.isDouble()) {
                parsed = val.toDouble();
                ok = true;
            } else if (val.isString()) {
                parsed = val.toString().toDouble(&ok);
            }
            if (ok && !std::isnan(parsed) && !std::isinf(parsed) && parsed >= 0.0) {
                m_lastValidPressure = parsed;
                foundNewData = true;
            }
        } else if (label == targetStatus) {
            if (val.isDouble()) {
                switch (val.toInt()) {
                    case 1: m_lastValidSensorStatus = QStringLiteral("Healthy"); break;
                    case 0: m_lastValidSensorStatus = QStringLiteral("Fault"); break;
                    default: m_lastValidSensorStatus = QStringLiteral("Unknown"); break;
                }
                foundNewData = true;
            } else if (val.isString()) {
                m_lastValidSensorStatus = val.toString();
                foundNewData = true;
            }
        }
    };

    if (doc.isObject()) {
        QJsonObject rootObj = doc.object();
        if (rootObj.contains(QStringLiteral("results")) && rootObj.value(QStringLiteral("results")).isArray()) {
            QJsonArray arr = rootObj.value(QStringLiteral("results")).toArray();
            for (const QJsonValue &item : arr) {
                if (item.isObject()) {
                    QJsonObject o = item.toObject();
                    QString label = o.value(QStringLiteral("label")).toString();
                    QJsonValue val = o.value(QStringLiteral("last_value"));
                    processPair(label, val);
                }
            }
        } else {
            // v2.0 dictionary mapping: {"waterlevel": {"value": 50.18}, "pressure": {"value": 842.98}, ...}
            for (auto it = rootObj.constBegin(); it != rootObj.constEnd(); ++it) {
                processPair(it.key(), it.value());
            }
        }
    } else if (doc.isArray()) {
        QJsonArray arr = doc.array();

        for (const QJsonValue &item : arr) {
            if (item.isObject()) {
                QJsonObject o = item.toObject();
                QString label = o.contains(QStringLiteral("label")) ? o.value(QStringLiteral("label")).toString() : (o.contains(QStringLiteral("variable")) ? o.value(QStringLiteral("variable")).toObject().value(QStringLiteral("label")).toString() : QString());
                QJsonValue val = o.contains(QStringLiteral("value")) ? o.value(QStringLiteral("value")) : o.value(QStringLiteral("last_value"));
                processPair(label, val);
            }
        }
    }

    if (foundNewData || m_hasReceivedFirstValidReading) {
        m_hasReceivedFirstValidReading = true;

        // Safe Diagnostic Logging (NEVER log authentication token or header)
        qDebug() << "[Ubidots] waterlevel  =" << m_lastValidWaterLevel;
        qDebug() << "[Ubidots] pressure    =" << m_lastValidPressure;
        qDebug() << "[Ubidots] temperature =" << m_lastValidTemperature;
        qDebug() << "[Ubidots] sensorstatus=" << m_lastValidSensorStatus;

        emit readingReceived(
            m_lastValidWaterLevel,
            m_lastValidTemperature,
            m_lastValidPressure,
            m_lastValidSensorStatus);
    }
}



} // namespace Panorama