#include <qute_note/utility/ShortcutManager.h>
#include "ShortcutManager_p.h"

namespace qute_note {

ShortcutManager::ShortcutManager(QObject * parent) :
    QObject(parent),
    d_ptr(new ShortcutManagerPrivate(*this))
{}

QKeySequence ShortcutManager::shortcut(const int key, const QString & context) const
{
    Q_D(const ShortcutManager);
    return d->shortcut(key, context);
}

QKeySequence ShortcutManager::shortcut(const QString & nonStandardKey, const QString & context) const
{
    Q_D(const ShortcutManager);
    return d->shortcut(nonStandardKey, context);
}

void ShortcutManager::setUserShortcut(int key, QKeySequence shortcut, QString context)
{
    Q_D(ShortcutManager);
    d->setUserShortcut(key, shortcut, context);
}

void ShortcutManager::setNonStandardUserShortcut(QString nonStandardKey, QKeySequence shortcut, QString context)
{
    Q_D(ShortcutManager);
    d->setNonStandardUserShortcut(nonStandardKey, shortcut, context);
}

void ShortcutManager::setDefaultShortcut(int key, QKeySequence shortcut, QString context)
{
    Q_D(ShortcutManager);
    d->setDefaultShortcut(key, shortcut, context);
}

void ShortcutManager::setNonStandardDefaultShortcut(QString nonStandardKey, QKeySequence shortcut, QString context)
{
    Q_D(ShortcutManager);
    d->setNonStandardDefaultShortcut(nonStandardKey, shortcut, context);
}

} // namespace qute_note
