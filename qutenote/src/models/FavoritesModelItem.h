#ifndef QUTE_NOTE_MODELS_FAVORITES_MODEL_ITEM_H
#define QUTE_NOTE_MODELS_FAVORITES_MODEL_ITEM_H

#include <qute_note/utility/Printable.h>

namespace qute_note {

class FavoritesModelItem: public Printable
{
public:
    struct Type
    {
        enum type
        {
            Notebook = 0,
            Tag,
            Note,
            SavedSearch,
            Unknown
        };
    };

public:
    explicit FavoritesModelItem(const Type::type type = Type::Unknown,
                                const QString & localUid = QString(),
                                const QString & displayName = QString(),
                                int numNotesTargeted = 0);

    Type::type type() const { return m_type; }
    void setType(const Type::type type) { m_type = type; }

    const QString & localUid() const { return m_localUid; }
    void setLocalUid(const QString & localUid) { m_localUid = localUid; }

    const QString & displayName() const { return m_displayName; }
    void setDisplayName(const QString & displayName) { m_displayName = displayName; }

    int numNotesTargeted() const { return m_numNotesTargeted; }
    void setNumNotesTargeted(const int numNotesTargeted) { m_numNotesTargeted = numNotesTargeted; }

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    Type::type      m_type;
    QString         m_localUid;
    QString         m_displayName;
    int             m_numNotesTargeted;
};

} // namespace qute_note

#endif // QUTE_NOTE_MODELS_FAVORITES_MODEL_ITEM_H
