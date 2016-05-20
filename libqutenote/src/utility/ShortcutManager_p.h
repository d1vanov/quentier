#ifndef LIB_QUTE_NOTE_UTILITY_SHORTCUT_MANAGER_PRIVATE_H
#define LIB_QUTE_NOTE_UTILITY_SHORTCUT_MANAGER_PRIVATE_H

#include <qute_note/utility/Qt4Helper.h>
#include <QObject>
#include <QKeySequence>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(ShortcutManager)

class ShortcutManagerPrivate: public QObject
{
    Q_OBJECT
public:
    explicit ShortcutManagerPrivate(ShortcutManager & shortcutManager);

    QKeySequence shortcut(const int key, const QString & context) const;
    QKeySequence shortcut(const QString & nonStandardKey, const QString & context) const;

Q_SIGNALS:
    void shortcutChanged(int key, QKeySequence shortcut, QString context);
    void nonStandardShortcutChanged(QString nonStandardKey, QKeySequence shortcut, QString context);

public Q_SLOTS:
    void setUserShortcut(int key, QKeySequence shortcut, QString context);
    void setNonStandardUserShortcut(QString nonStandardKey, QKeySequence shortcut, QString context);

    void setDefaultShortcut(int key, QKeySequence shortcut, QString context);
    void setNonStandardDefaultShortcut(QString nonStandardKey, QKeySequence shortcut, QString context);

private:
    QKeySequence defaultShortcut(const int key, const QString & keyString, const QString & context) const;
    QKeySequence defaultShortcut(const QString & nonStandardKey, const QString & context) const;

    QString keyToString(const int key) const;
    QString shortcutGroupString(const QString & context, const bool defaultShortcut, const bool nonStandardShortcut) const;

private:
    ShortcutManager * const q_ptr;
    Q_DECLARE_PUBLIC(ShortcutManager)
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_UTILITY_SHORTCUT_MANAGER_PRIVATE_H
