#ifndef __QUTE_NOTE__CORE__CLIENT__SYNCHRONIZATION__SYNCHRONIZATION_MANAGER_H
#define __QUTE_NOTE__CORE__CLIENT__SYNCHRONIZATION__SYNCHRONIZATION_MANAGER_H

#include <tools/qt4helper.h>
#include <QObject>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThread)
QT_FORWARD_DECLARE_CLASS(SynchronizationManagerPrivate)

class SynchronizationManager: public QObject
{
    Q_OBJECT
public:
    SynchronizationManager(LocalStorageManagerThread & localStorageManagerThread);
    virtual ~SynchronizationManager();

    void synchronize();

private:
    SynchronizationManager() Q_DECL_DELETE;
    SynchronizationManager(const SynchronizationManager & other) Q_DECL_DELETE;
    SynchronizationManager & operator=(const SynchronizationManager & other) Q_DECL_DELETE;

    SynchronizationManagerPrivate * d_ptr;
    Q_DECLARE_PRIVATE(SynchronizationManager)
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__SYNCHRONIZATION__SYNCHRONIZATION_MANAGER_H
