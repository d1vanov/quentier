#ifndef __QUTE_NOTE__MODELS__NOTEBOOK_ITEM_H
#define __QUTE_NOTE__MODELS__NOTEBOOK_ITEM_H

#include <qute_note/utility/Printable.h>

namespace qute_note {

class NotebookItem: public Printable
{
public:
    explicit NotebookItem(const QString & localUid = QString(),
                          const QString & guid = QString(),
                          const QString & name = QString(),
                          const QString & stack = QString(),
                          const bool isSynchronizable = false,
                          const bool isUpdatable = false,
                          const bool isDirty = false,
                          const bool isDefault = false,
                          const bool isPublished = false,
                          const bool isLinkedNotebook = false);
    ~NotebookItem();

    const QString & localUid() const { return m_localUid; }
    void setLocalUid(const QString & localUid) { m_localUid = localUid; }

    const QString & guid() const { return m_guid; }
    void setGuid(const QString & guid) { m_guid = guid; }

    QString nameUpper() const { return m_name.toUpper(); }

    const QString & stack() const { return m_stack; }
    void setStack(const QString & stack) { m_stack = stack; }

    const QString & name() const { return m_name; }
    void setName(const QString & name) { m_name = name; }

    bool isSynchronizable() const { return m_isSynchronizable; }
    void setSynchronizable(const bool synchronizable) { m_isSynchronizable = synchronizable; }

    bool isUpdatable() const { return m_isUpdatable; }
    void setUpdatable(const bool updatable) { m_isUpdatable = updatable; }

    bool isDirty() const { return m_isDirty; }
    void setDirty(const bool dirty) { m_isDirty = dirty; }

    bool isDefault() const { return m_isDefault; }
    void setDefault(const bool flag) { m_isDefault = flag; }

    bool isPublished() const { return m_isPublished; }
    void setPublished(const bool flag) { m_isPublished = flag; }

    bool isLinkedNotebook() const { return m_isLinkedNotebook; }
    void setLinkedNotebook(const bool flag) { m_isLinkedNotebook = flag; }

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    QString     m_localUid;
    QString     m_guid;
    QString     m_name;
    QString     m_stack;
    bool        m_isSynchronizable;
    bool        m_isUpdatable;
    bool        m_isDirty;
    bool        m_isDefault;
    bool        m_isPublished;
    bool        m_isLinkedNotebook;
};

} // namespace qute_note

#endif // __QUTE_NOTE__MODELS__NOTEBOOK_ITEM_H
