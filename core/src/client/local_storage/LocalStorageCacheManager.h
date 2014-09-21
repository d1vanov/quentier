#ifndef __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_CACHE_MANAGER_H
#define __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_CACHE_MANAGER_H

#include <QScopedPointer>
#include <functional>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(Note)

QT_FORWARD_DECLARE_CLASS(LocalStorageCacheManagerPrivate)
class LocalStorageCacheManager
{
public:
    LocalStorageCacheManager();
    ~LocalStorageCacheManager();

    size_t numCachedNotes() const;
    void cacheNote(const Note & note);

    void installCacheExpiryFunction(const std::function<bool(const LocalStorageCacheManager &)> & function);

private:
    LocalStorageCacheManager(const LocalStorageCacheManager & other) = delete;
    LocalStorageCacheManager(LocalStorageCacheManager && other) = delete;
    LocalStorageCacheManager & operator=(const LocalStorageCacheManager & other) = delete;
    LocalStorageCacheManager & operator=(LocalStorageCacheManager && other) = delete;

    Q_DECLARE_PRIVATE(LocalStorageCacheManager)
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_CACHE_MANAGER_H
