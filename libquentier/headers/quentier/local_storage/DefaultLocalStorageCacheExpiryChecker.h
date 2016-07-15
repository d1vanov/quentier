/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIB_QUENTIER_LOCAL_STORAGE_DEFAULT_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H
#define LIB_QUENTIER_LOCAL_STORAGE_DEFAULT_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H

#include <quentier/local_storage/ILocalStorageCacheExpiryChecker.h>

#define MAX_NOTES_TO_STORE 100
#define MAX_NOTEBOOKS_TO_STORE 20
#define MAX_TAGS_TO_STORE 200
#define MAX_LINKED_NOTEBOOKS_TO_STORE 20
#define MAX_SAVED_SEARCHES_TO_STORE 20

namespace quentier {

class QUENTIER_EXPORT DefaultLocalStorageCacheExpiryChecker: public ILocalStorageCacheExpiryChecker
{
public:
    DefaultLocalStorageCacheExpiryChecker(const LocalStorageCacheManager & cacheManager);
    virtual ~DefaultLocalStorageCacheExpiryChecker();

    virtual DefaultLocalStorageCacheExpiryChecker * clone() const Q_DECL_OVERRIDE;

    virtual bool checkNotes() const Q_DECL_OVERRIDE;
    virtual bool checkNotebooks() const Q_DECL_OVERRIDE;
    virtual bool checkTags() const Q_DECL_OVERRIDE;
    virtual bool checkLinkedNotebooks() const Q_DECL_OVERRIDE;
    virtual bool checkSavedSearches() const Q_DECL_OVERRIDE;

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    DefaultLocalStorageCacheExpiryChecker() Q_DECL_DELETE;
    Q_DISABLE_COPY(DefaultLocalStorageCacheExpiryChecker)
};

} // namespace quentier

#endif // LIB_QUENTIER_LOCAL_STORAGE_DEFAULT_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H
