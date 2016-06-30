#ifndef LIB_QUENTIER_NOTE_EDITOR_SPELL_CHECKER_DICTIONARIES_FINDER_H
#define LIB_QUENTIER_NOTE_EDITOR_SPELL_CHECKER_DICTIONARIES_FINDER_H

#include <quentier/utility/Qt4Helper.h>
#include <QObject>
#include <QRunnable>
#include <QHash>
#include <QString>

namespace quentier {

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

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_SPELL_CHECKER_DICTIONARIES_FINDER_H
