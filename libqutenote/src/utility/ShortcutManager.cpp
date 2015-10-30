#include <qute_note/utility/ShortcutManager.h>
#include "ShortcutManager_p.h"

namespace qute_note {

ShortcutManager::ShortcutManager(QObject * parent) :
    QObject(parent),
    d_ptr(new ShortcutManagerPrivate(*this))
{}

QKeySequence ShortcutManager::shortcut(const QKeySequence::StandardKey standardKey, const QString & context) const
{
    Q_D(const ShortcutManager);
    return d->shortcut(standardKey, context);
}

QKeySequence ShortcutManager::shortcut(const QString & nonStandardKey, const QString & context) const
{
    Q_D(const ShortcutManager);
    return d->shortcut(nonStandardKey, context);
}

void ShortcutManager::setUserShortcut(QKeySequence::StandardKey standardKey, QKeySequence shortcut, QString context)
{
    Q_D(ShortcutManager);
    d->setUserShortcut(standardKey, shortcut, context);
}

void ShortcutManager::setNonStandardUserShortcut(QString nonStandardKey, QKeySequence shortcut, QString context)
{
    Q_D(ShortcutManager);
    d->setNonStandardUserShortcut(nonStandardKey, shortcut, context);
}

} // namespace qute_note
