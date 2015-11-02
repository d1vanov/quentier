#ifndef __LIB_QUTE_NOTE__UTILITY__SHORTCUT_MANAGER_H
#define __LIB_QUTE_NOTE__UTILITY__SHORTCUT_MANAGER_H

#include <qute_note/utility/Linkage.h>
#include <qute_note/utility/Qt4Helper.h>
#include <QObject>
#include <QKeySequence>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(ShortcutManagerPrivate)

class QUTE_NOTE_EXPORT ShortcutManager: public QObject
{
    Q_OBJECT
public:
    explicit ShortcutManager(QObject * parent = Q_NULLPTR);

    QKeySequence shortcut(const QKeySequence::StandardKey standardKey, const QString & context = QString()) const;
    QKeySequence shortcut(const QString & nonStandardKey, const QString & context = QString()) const;

Q_SIGNALS:
    void shortcutChanged(QKeySequence::StandardKey standardKey, QKeySequence shortcut, QString context);
    void nonStandardShortcutChanged(QString nonStandardKey, QKeySequence shortcut, QString context);

public Q_SLOTS:
    void setUserShortcut(QKeySequence::StandardKey standardKey, QKeySequence shortcut, QString context = QString());
    void setNonStandardUserShortcut(QString nonStandardKey, QKeySequence shortcut, QString context = QString());

    void setDefaultShortcut(QKeySequence::StandardKey standardKey, QKeySequence shortcut, QString context = QString());
    void setNonStandardDefaultShortcut(QString nonStandardKey, QKeySequence shortcut, QString context = QString());

private:
    ShortcutManagerPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(ShortcutManager)
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__UTILITY__SHORTCUT_MANAGER_H

