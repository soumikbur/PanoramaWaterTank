#pragma once

#include <QObject>

namespace Panorama {

/*!
 * \brief Composition root - the one class allowed to know about every
 *        other backend class (docs/05-implementation-blueprint.md,
 *        Section 1).
 *
 * Constructs and owns ApiClient, ConnectionManager, TankRepository, and
 * ApplicationStateManager, in the order fixed by
 * docs/05-implementation-blueprint.md, Section 13. That wiring, along
 * with the currentPage / navigateTo() / retryConnection() surface
 * (docs/05, Section 1), is implemented in a later milestone; this class
 * currently establishes its place as the eventual composition root.
 */
class DashboardController : public QObject
{
    Q_OBJECT

public:
    explicit DashboardController(QObject *parent = nullptr);
};

} // namespace Panorama
