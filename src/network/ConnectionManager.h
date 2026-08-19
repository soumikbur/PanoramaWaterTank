// ConnectionManager.h
#ifndef PANORAMA_CONNECTIONMANAGER_H
#define PANORAMA_CONNECTIONMANAGER_H

#include <QObject>
#include <QString>

namespace Panorama {

class ApiClient;
class TankRepository;
class TankModel;
class Logger;

class ConnectionManager : public QObject
{
    Q_OBJECT

public:
    explicit ConnectionManager(Logger &logger, QObject *parent = nullptr);

    void setTankModel(TankModel *model);

    void setApiConfiguration(const QString &baseUrl,
                             const QString &token,
                             const QString &deviceLabel,
                             int pollingIntervalMs,
                             int transferTimeoutMs,
                             const QString &levelVar,
                             const QString &tempVar,
                             const QString &pressureVar,
                             const QString &statusVar);

    void start();
    void stop();

private:
    Logger &m_logger;
    ApiClient *m_apiClient;
    TankRepository *m_tankRepository;
    TankModel *m_tankModel{nullptr};
};

} // namespace Panorama

#endif // PANORAMA_CONNECTIONMANAGER_H