#ifndef __QUTE_NOTE__MODELS__NOTEBOOK_ITEM_H
#define __QUTE_NOTE__MODELS__NOTEBOOK_ITEM_H

#include <qute_note/utility/Printable.h>
#include <QDataStream>

namespace qute_note {

class NotebookItem: public Printable
{
public:
    explicit NotebookItem(const QString & localUid = QString(),
                          const QString & guid = QString(),
                          const QString & name = QString(),
                          const QString & stack = QString(),
                          const bool isSynchronizable = false,
                          const bool isDirty = false);
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

    bool isDirty() const { return m_isDirty; }
    void setDirty(const bool dirty) { m_isDirty = dirty; }

    virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;

    friend QDataStream & operator<<(QDataStream & out, const NotebookItem & item);
    friend QDataStream & operator>>(QDataStream & in, NotebookItem & item);

private:
    QString     m_localUid;
    QString     m_guid;
    QString     m_name;
    QString     m_stack;
    bool        m_isSynchronizable;
    bool        m_isDirty;
};

} // namespace qute_note

#endif // __QUTE_NOTE__MODELS__NOTEBOOK_ITEM_H
