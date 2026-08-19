#pragma once

#include <QObject>

namespace Panorama {

/*!
 * \brief Formal application-lifecycle state machine
 *        (docs/05-implementation-blueprint.md, Section 1;
 *        docs/02-software-architecture.md, Section 6).
 *
 * The enumerated states (see Panorama::ApplicationState in
 * src/backend/Types.h) and their valid-transition table are implemented
 * in a later milestone; this class currently establishes its place in
 * the backend object graph.
 */
class ApplicationStateManager : public QObject
{
    Q_OBJECT

public:
    explicit ApplicationStateManager(QObject *parent = nullptr);
};

} // namespace Panorama
