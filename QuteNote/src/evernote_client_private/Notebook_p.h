#ifndef __QUTE_NOTE__EVERNOTE_CLIENT_PRIVATE__NOTEBOOK_P_H
#define __QUTE_NOTE__EVERNOTE_CLIENT_PRIVATE__NOTEBOOK_P_H

#include "../evernote_client/Notebook.h"

namespace qute_note {

class NotebookPrivate
{
public:
    NotebookPrivate() = default;
    NotebookPrivate(const NotebookPrivate & other) = default;
    NotebookPrivate(NotebookPrivate && other) = default;
    NotebookPrivate & operator=(const NotebookPrivate & other);
    NotebookPrivate & operator=(NotebookPrivate && other);

    QString  m_name;
    bool     m_isDefault;
    time_t   m_creationTimestamp;
    time_t   m_modificationTimestamp;
    Notebook::NoteSortOrder        m_noteSortOrder;
    Notebook::PublicNoteSortOrder  m_publicNoteSortOrder;
    QString  m_publicDescription;
    bool     m_isPublished;
    QString  m_containerName;
    bool     m_isLastUsed;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT_PRIVATE__NOTEBOOK_P_H
