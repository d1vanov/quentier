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

#include <quentier/local_storage/DefaultLocalStorageCacheExpiryChecker.h>
#include <quentier/local_storage/LocalStorageCacheManager.h>

namespace quentier {

DefaultLocalStorageCacheExpiryChecker::DefaultLocalStorageCacheExpiryChecker(const LocalStorageCacheManager & cacheManager) :
    ILocalStorageCacheExpiryChecker(cacheManager)
{}

DefaultLocalStorageCacheExpiryChecker::~DefaultLocalStorageCacheExpiryChecker()
{}

DefaultLocalStorageCacheExpiryChecker * DefaultLocalStorageCacheExpiryChecker::clone() const
{
    return new DefaultLocalStorageCacheExpiryChecker(m_localStorageCacheManager);
}

// Probably should not attempt to track memory using syscalls for the following methods, too expensive

bool DefaultLocalStorageCacheExpiryChecker::checkNotes() const
{
    size_t numNotes = m_localStorageCacheManager.numCachedNotes();
    return (numNotes < MAX_NOTES_TO_STORE);
}

bool DefaultLocalStorageCacheExpiryChecker::checkNotebooks() const
{
    size_t numNotebooks = m_localStorageCacheManager.numCachedNotebooks();
    return (numNotebooks < MAX_NOTEBOOKS_TO_STORE);
}

bool DefaultLocalStorageCacheExpiryChecker::checkTags() const
{
    size_t numTags = m_localStorageCacheManager.numCachedTags();
    return (numTags < MAX_TAGS_TO_STORE);
}

bool DefaultLocalStorageCacheExpiryChecker::checkLinkedNotebooks() const
{
    size_t numCachedLinkedNotebooks = m_localStorageCacheManager.numCachedLinkedNotebooks();
    return (numCachedLinkedNotebooks < MAX_LINKED_NOTEBOOKS_TO_STORE);
}

bool DefaultLocalStorageCacheExpiryChecker::checkSavedSearches() const
{
    size_t numCachedSavedSearches = m_localStorageCacheManager.numCachedSavedSearches();
    return (numCachedSavedSearches < MAX_SAVED_SEARCHES_TO_STORE);
}

QTextStream & DefaultLocalStorageCacheExpiryChecker::print(QTextStream & strm) const
{
    const char * indent = "  ";

    strm << QStringLiteral("DefaultLocalStorageCacheExpiryChecker: {\n") ;
    strm << indent << QStringLiteral("max notes to store: ") << MAX_NOTES_TO_STORE << QStringLiteral(";\n");
    strm << indent << QStringLiteral("max notebooks to store: ") << MAX_NOTEBOOKS_TO_STORE << QStringLiteral(";\n");
    strm << indent << QStringLiteral("max tags to store: ") << MAX_TAGS_TO_STORE << QStringLiteral(";\n");
    strm << indent << QStringLiteral("max linked notebooks to store: ") << MAX_LINKED_NOTEBOOKS_TO_STORE << QStringLiteral(";\n");
    strm << indent << QStringLiteral("max saved searches to store: ") << MAX_SAVED_SEARCHES_TO_STORE << QStringLiteral("\n");
    strm << QStringLiteral("};\n");

    return strm;
}

} // namespace quentier
