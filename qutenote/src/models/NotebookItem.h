#ifndef __QUTE_NOTE__MODELS__NOTEBOOK_ITEM_H
#define __QUTE_NOTE__MODELS__NOTEBOOK_ITEM_H

#include <qute_note/utility/Printable.h>
#include <QDataStream>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(NotebookModelItem)

class NotebookItem: public Printable
{
public:
    explicit NotebookItem(const QString & localUid = QString(),
                          const QString & guid = QString(),
                          const QString & name = QString(),
                          const bool isSynchronizable = false,
                          const bool isDirty = false,
                          const NotebookModelItem * parent = Q_NULLPTR);
    ~NotebookItem();

    const QString & localUid() const { return m_localUid; }
    void setLocalUid(const QString & localUid) { m_localUid = localUid; }

    const QString & guid() const { return m_guid; }
    void setGuid(const QString & guid) { m_guid = guid; }

    QString nameUpper() const { return m_name.toUpper(); }

    const QString & name() const { return m_name; }
    void setName(const QString & name) { m_name = name; }

    bool isSynchronizable() const { return m_isSynchronizable; }
    void setSynchronizable(const bool synchronizable) { m_isSynchronizable = synchronizable; }

    bool isDirty() const { return m_isDirty; }
    void setDirty(const bool dirty) { m_isDirty = dirty; }

    const NotebookModelItem * parent() const { return m_parent; }
    void setParent(const NotebookModelItem * parent) const;

    virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;

    friend QDataStream & operator<<(QDataStream & out, const NotebookItem & item);
    friend QDataStream & operator>>(QDataStream & in, NotebookItem & item);

private:
    QString     m_localUid;
    QString     m_guid;
    QString     m_name;
    bool        m_isSynchronizable;
    bool        m_isDirty;

    // NOTE: this is mutable in order to have the possibility to organize
    // the efficient storage of TagModelItems in boost::multi_index_container:
    // it doesn't allow the direct modification of its stored items,
    // however, these pointers to parent and children don't really affect
    // that container's indices
    mutable const NotebookModelItem *     m_parent;
};

} // namespace qute_note

#endif // __QUTE_NOTE__MODELS__NOTEBOOK_ITEM_H
