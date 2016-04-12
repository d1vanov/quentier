#include <qute_note/local_storage/DefaultLocalStorageCacheExpiryChecker.h>
#include <qute_note/local_storage/LocalStorageCacheManager.h>

namespace qute_note {

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
    QString indent = "  ";

    strm << "DefaultLocalStorageCacheExpiryChecker: {\n" ;
    strm << indent << "max notes to store: " << MAX_NOTES_TO_STORE << ";\n";
    strm << indent << "max notebooks to store: " << MAX_NOTEBOOKS_TO_STORE << ";\n";
    strm << indent << "max tags to store: " << MAX_TAGS_TO_STORE << ";\n";
    strm << indent << "max linked notebooks to store: " << MAX_LINKED_NOTEBOOKS_TO_STORE << ";\n";
    strm << indent << "max saved searches to store: " << MAX_SAVED_SEARCHES_TO_STORE << "\n";
    strm << "};\n";

    return strm;
}

} // namespace qute_note
