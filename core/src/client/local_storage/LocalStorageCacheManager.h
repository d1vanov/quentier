#ifndef __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_CACHE_MANAGER_H
#define __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_CACHE_MANAGER_H

#include <QSharedPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(Note)

class LocalStorageCacheManager
{
public:
    LocalStorageCacheManager();
    ~LocalStorageCacheManager();

    // TODO: implement

private:
    LocalStorageCacheManager(const LocalStorageCacheManager & other) = delete;
    LocalStorageCacheManager(LocalStorageCacheManager && other) = delete;
    LocalStorageCacheManager & operator=(const LocalStorageCacheManager & other) = delete;
    LocalStorageCacheManager & operator=(LocalStorageCacheManager && other) = delete;
};

}

#endif // __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_CACHE_MANAGER_H
