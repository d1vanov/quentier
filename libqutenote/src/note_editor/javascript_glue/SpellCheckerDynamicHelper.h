#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__SPELL_CHECKER_DYNAMIC_HELPER_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__SPELL_CHECKER_DYNAMIC_HELPER_H

#include <qute_note/utility/Qt4Helper.h>
#include <QObject>
#include <QStringList>
#include <QVariant>

namespace qute_note {

class SpellCheckerDynamicHelper: public QObject
{
    Q_OBJECT
public:
    explicit SpellCheckerDynamicHelper(QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void lastEnteredWords(QStringList words);

public Q_SLOTS:

    // NOTE: workarounding https://bugreports.qt.io/browse/QTBUG-39951 - JavaScript array doesn't get automatically converted to QVariant
#ifdef USE_QT_WEB_ENGINE
    void setLastEnteredWords(QVariant words);
#else
    void setLastEnteredWords(QVariantList words);
#endif
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__SPELL_CHECKER_DYNAMIC_HELPER_H
