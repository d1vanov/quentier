#include "DatabaseManager.h"

#ifdef USE_QT5
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif

namespace qute_note {

DatabaseManager::DatabaseManager() :
    m_applicationPersistenceStoragePath(QDesktopServices::storageLocation(QDesktopServices::DataLocation))
{
    m_sqlDatabase.addDatabase("QSQLITE");
    m_sqlDatabase.setDatabaseName(m_applicationPersistenceStoragePath + QString("/qn.storage.sqlite"));

    if (!m_sqlDatabase.open()) {
        // TODO: create and throw appropriate exception
    }
}

DatabaseManager::~DatabaseManager()
{
    if (m_sqlDatabase.open()) {
        m_sqlDatabase.close();
    }
}

}
