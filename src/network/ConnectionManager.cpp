// ConnectionManager.cpp
#include "ConnectionManager.h"
#include "ApiClient.h"
#include "TankRepository.h"
#include "TankModel.h"
#include "Logger.h"

namespace Panorama {

ConnectionManager::ConnectionManager(Logger &logger, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
    , m_apiClient(new ApiClient(this))
    , m_tankRepository(new TankRepository(logger, this))
{
    connect(m_apiClient, &ApiClient::readingReceived,
            m_tankRepository, &TankRepository::processReading);

    connect(m_apiClient, &ApiClient::connectionStateChanged, this, [this](bool connected) {
        if (m_tankModel) {
            m_tankModel->setConnectionState(connected ? QStringLiteral("Connected") : QStringLiteral("Offline"));
        }
    });

    connect(m_apiClient, &ApiClient::historicalDataReceived, this, [this](const QVariantList &history) {
        if (m_tankModel) {
            m_tankModel->setTrendHistory(history);
        }
    });

    connect(m_apiClient, &ApiClient::networkError, this, [this](const QString &error) {
        m_logger.logError(Logger::LogLevel::Warning, QStringLiteral("ConnectionManager Network Error: %1").arg(error));
    });
}


void ConnectionManager::setTankModel(TankModel *model)
{
    if (!model) {
        return;
    }
    m_tankModel = model;
    m_tankRepository->setTankModel(m_tankModel);

    connect(m_tankModel, &TankModel::historyRangeHoursChanged,
            m_apiClient, &ApiClient::setHistoryRangeHours);

    connect(m_tankModel, &TankModel::trendRefreshRequested,
            m_apiClient, &ApiClient::fetchHistoricalData);

    // Synchronize initial range
    m_apiClient->setHistoryRangeHours(m_tankModel->historyRangeHours());
}

void ConnectionManager::setApiConfiguration(const QString &baseUrl,
                                            const QString &token,
                                            const QString &deviceLabel,
                                            int pollingIntervalMs,
                                            int transferTimeoutMs,
                                            const QString &levelVar,
                                            const QString &tempVar,
                                            const QString &pressureVar,
                                            const QString &statusVar)
{
    m_apiClient->setApiConfiguration(baseUrl, token, deviceLabel, pollingIntervalMs, transferTimeoutMs, levelVar, tempVar, pressureVar, statusVar);
}

void ConnectionManager::start()
{
    if (m_tankModel) {
        m_tankModel->setConnectionState(QStringLiteral("Connecting"));
    }

    m_apiClient->startPolling();
}

void ConnectionManager::stop()
{
    m_apiClient->stopPolling();

    if (m_tankModel) {
        m_tankModel->setConnectionState(QStringLiteral("Offline"));
    }
}

} // namespace Panorama