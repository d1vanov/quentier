#include "ShortcutManager_p.h"
#include <qute_note/utility/ShortcutManager.h>
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

ShortcutManagerPrivate::ShortcutManagerPrivate(ShortcutManager & shortcutManager) :
    QObject(&shortcutManager),
    q_ptr(&shortcutManager)
{
    Q_Q(ShortcutManager);

    QObject::connect(this, QNSIGNAL(ShortcutManagerPrivate,userShortcutChanged,QKeySequence::StandardKey,QKeySequence,QString),
                     q, QNSIGNAL(ShortcutManager,userShortcutChanged,QKeySequence::StandardKey,QKeySequence,QString));
    QObject::connect(this, QNSIGNAL(ShortcutManagerPrivate,userNonStandardShortcutChanged,QString,QKeySequence,QString),
                     q, QNSIGNAL(ShortcutManager,userNonStandardShortcutChanged,QString,QKeySequence,QString));
}

QKeySequence ShortcutManagerPrivate::shortcut(const QKeySequence::StandardKey standardKey, const QString & context) const
{
    QNDEBUG("ShortcutManagerPrivate::shortcut: standard key = " << standardKey << ", context = " << context);
    // TODO: implement
    return QKeySequence();
}

QKeySequence ShortcutManagerPrivate::shortcut(const QString & nonStandardKey, const QString & context) const
{
    QNDEBUG("ShortcutManagerPrivate::shortcut: non standard key = " << nonStandardKey << ", context = " << context);
    // TODO: implement
    return QKeySequence();
}

void ShortcutManagerPrivate::setUserShortcut(QKeySequence::StandardKey standardKey, QKeySequence shortcut, QString context)
{
    QNDEBUG("ShortcutManagerPrivate::setUserShortcut: standard key = " << standardKey << ", shortcut = " << shortcut
            << ", context = " << context);
    // TODO: implement
}

void ShortcutManagerPrivate::setNonStandardUserShortcut(QString nonStandardKey, QKeySequence shortcut, QString context)
{
    QNDEBUG("ShortcutManagerPrivate::setNonStandardUserShortcut: non-standard key = " << nonStandardKey << ", shortcut = "
            << shortcut << ", context = " << context);
    // TODO: implement
}

} // namespace qute_note
