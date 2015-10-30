#ifndef __LIB_QUTE_NOTE__UTILITY__SHORTCUT_MANAGER_PRIVATE_H
#define __LIB_QUTE_NOTE__UTILITY__SHORTCUT_MANAGER_PRIVATE_H

#include <qute_note/utility/Qt4Helper.h>
#include <QObject>
#include <QKeySequence>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(ShortcutManager);

class ShortcutManagerPrivate: public QObject
{
    Q_OBJECT
public:
    explicit ShortcutManagerPrivate(ShortcutManager & shortcutManager);

    QKeySequence shortcut(const QKeySequence::StandardKey standardKey, const QString & context) const;
    QKeySequence shortcut(const QString & nonStandardKey, const QString & context) const;

Q_SIGNALS:
    void userShortcutChanged(QKeySequence::StandardKey standardKey, QKeySequence shortcut, QString context);
    void userNonStandardShortcutChanged(QString nonStandardKey, QKeySequence shortcut, QString context);

public Q_SLOTS:
    void setUserShortcut(QKeySequence::StandardKey standardKey, QKeySequence shortcut, QString context);
    void setNonStandardUserShortcut(QString nonStandardKey, QKeySequence shortcut, QString context);

private:
    QKeySequence defaultShortcut(const QKeySequence::StandardKey standardKey, const QString & context) const;
    QKeySequence defaultShortcut(const QString & nonStandardKey, const QString & context) const;

    QString standardKeyToString(const QKeySequence::StandardKey standardKey) const;

private:
    ShortcutManager * const q_ptr;
    Q_DECLARE_PUBLIC(ShortcutManager);
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__UTILITY__SHORTCUT_MANAGER_PRIVATE_H
