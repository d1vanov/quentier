#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__SPELL_CHECKER_DICTIONARIES_FINDER_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__SPELL_CHECKER_DICTIONARIES_FINDER_H

#include <qute_note/utility/Qt4Helper.h>
#include <QObject>
#include <QRunnable>
#include <QHash>
#include <QString>

namespace qute_note {

class SpellCheckerDictionariesFinder: public QObject,
                                      public QRunnable
{
    Q_OBJECT
public:
    typedef QHash<QString, QPair<QString, QString> > DicAndAffFilesByDictionaryName;

public:
    SpellCheckerDictionariesFinder(QObject * parent = Q_NULLPTR);

    virtual void run() Q_DECL_OVERRIDE;

Q_SIGNALS:
    void foundDictionaries(DicAndAffFilesByDictionaryName docAndAffFilesByDictionaryName);

private:
    DicAndAffFilesByDictionaryName  m_files;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__SPELL_CHECKER_DICTIONARIES_FINDER_H
