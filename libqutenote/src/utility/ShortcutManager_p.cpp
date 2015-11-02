#include "ShortcutManager_p.h"
#include <qute_note/utility/ShortcutManager.h>
#include <qute_note/utility/ApplicationSettings.h>
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

ShortcutManagerPrivate::ShortcutManagerPrivate(ShortcutManager & shortcutManager) :
    QObject(&shortcutManager),
    q_ptr(&shortcutManager)
{
    Q_Q(ShortcutManager);

    QObject::connect(this, QNSIGNAL(ShortcutManagerPrivate,shortcutChanged,QKeySequence::StandardKey,QKeySequence,QString),
                     q, QNSIGNAL(ShortcutManager,shortcutChanged,QKeySequence::StandardKey,QKeySequence,QString));
    QObject::connect(this, QNSIGNAL(ShortcutManagerPrivate,nonStandardShortcutChanged,QString,QKeySequence,QString),
                     q, QNSIGNAL(ShortcutManager,nonStandardShortcutChanged,QString,QKeySequence,QString));
}

QKeySequence ShortcutManagerPrivate::shortcut(const QKeySequence::StandardKey standardKey, const QString & context) const
{
    QString standardKeyString = standardKeyToString(standardKey);

    QNDEBUG("ShortcutManagerPrivate::shortcut: standard key = " << standardKeyString << " ("
            << standardKey << "), context = " << context);

    ApplicationSettings settings;

    settings.beginGroup(shortcutGroupString(context, /* default shortcut = */ false, /* non-standard shortcut = */ false));
    QVariant value = settings.value(standardKeyString);
    QNTRACE("Read from app settings: " << value);
    settings.endGroup();

    if (!value.isValid()) {
        QNTRACE("Couldn't find user shortcut for standard key " << standardKeyString << " (" << standardKey << "), will use the default one");
        return defaultShortcut(standardKey, standardKeyString, context);
    }

    if (value.type() != QVariant::KeySequence)
    {
        if (!value.canConvert(QVariant::KeySequence)) {
            QNWARNING("Found user shortcut is not convertible to key sequence: " << value << "; fallback to the default shortcut");
            return defaultShortcut(standardKey, standardKeyString, context);
        }

        Q_UNUSED(value.convert(QVariant::KeySequence))
        QNTRACE("Converted to key sequence: " << value);
    }

    return qvariant_cast<QKeySequence>(value);
}

QKeySequence ShortcutManagerPrivate::shortcut(const QString & nonStandardKey, const QString & context) const
{
    QNDEBUG("ShortcutManagerPrivate::shortcut: non standard key = " << nonStandardKey << ", context = " << context);

    ApplicationSettings settings;

    settings.beginGroup(shortcutGroupString(context, /* default shortcut = */ false, /* non-standard shortcut = */ true));
    QVariant value = settings.value(nonStandardKey);
    QNTRACE("Read from app settings: " << value);
    settings.endGroup();

    if (!value.isValid()) {
        QNTRACE("Couldn't find user shortcut for non-standard key " << nonStandardKey << ", will use the default one");
        return defaultShortcut(nonStandardKey, context);
    }

    if (value.type() != QVariant::KeySequence)
    {
        if (!value.canConvert(QVariant::KeySequence)) {
            QNWARNING("Found user shortcut not convertible to key sequence: " << value << "; fallback to the default shortcut");
            return defaultShortcut(nonStandardKey, context);
        }

        Q_UNUSED(value.convert(QVariant::KeySequence))
        QNTRACE("Converted to key sequence: " << value);
    }

    return qvariant_cast<QKeySequence>(value);
}

void ShortcutManagerPrivate::setUserShortcut(QKeySequence::StandardKey standardKey, QKeySequence shortcut, QString context)
{
    QString standardKeyString = standardKeyToString(standardKey);

    QNDEBUG("ShortcutManagerPrivate::setUserShortcut: standard key = " << standardKeyString << " (" << standardKey
            << "), shortcut = " << shortcut << ", context = " << context);

    ApplicationSettings settings;
    settings.beginGroup(shortcutGroupString(context, /* default shortcut = */ false, /* non-standard shortcut = */ false));
    settings.setValue(standardKeyString, shortcut.operator QVariant());  // Need this esoteric syntax to make compilers happy with Qt4
    settings.endGroup();

    emit shortcutChanged(standardKey, shortcut, context);
}

void ShortcutManagerPrivate::setNonStandardUserShortcut(QString nonStandardKey, QKeySequence shortcut, QString context)
{
    QNDEBUG("ShortcutManagerPrivate::setNonStandardUserShortcut: non-standard key = " << nonStandardKey << ", shortcut = "
            << shortcut << ", context = " << context);

    ApplicationSettings settings;
    settings.beginGroup(shortcutGroupString(context, /* default shortcut = */ false, /* non-standard shortcut = */ true));
    settings.setValue(nonStandardKey, shortcut.operator QVariant());   // Need this esoteric syntax to make compilers happy with Qt4
    settings.endGroup();

    emit nonStandardShortcutChanged(nonStandardKey, shortcut, context);
}

void ShortcutManagerPrivate::setDefaultShortcut(QKeySequence::StandardKey standardKey, QKeySequence shortcut, QString context)
{
    QString standardKeyString = standardKeyToString(standardKey);

    QNDEBUG("ShortcutManagerPrivate::setDefaultShortcut: standard key = " << standardKeyString << " (" << standardKey
            << "), shortcut = " << shortcut << ", context = " << context);

    ApplicationSettings settings;
    settings.beginGroup(shortcutGroupString(context, /* default shortcut = */ true, /* non-standard shortcut = */ false));
    settings.setValue(standardKeyString, shortcut.operator QVariant());   // Need this esoteric syntax to make compilers happy with Qt4
    settings.endGroup();

    // Need to emit the notification is there's no user shortcut overriding the default one
    settings.beginGroup(shortcutGroupString(context, /* default shortcut = */ false, /* non-standard shortcut = */ false));
    QVariant userShortcut = settings.value(standardKeyString);
    settings.endGroup();

    if (!userShortcut.isValid() || ((userShortcut.type() != QVariant::KeySequence) && !userShortcut.canConvert(QVariant::KeySequence))) {
        QNTRACE("Found no user shortcut overriding the default one");
        emit shortcutChanged(standardKey, shortcut, context);
    }
}

void ShortcutManagerPrivate::setNonStandardDefaultShortcut(QString nonStandardKey, QKeySequence shortcut, QString context)
{
    QNDEBUG("ShortcutManagerPrivate::setNonStandardDefaultShortcut: non-standard key = " << nonStandardKey
            << ", shortcut = " << shortcut << ", context = " << context);

    ApplicationSettings settings;
    settings.beginGroup(shortcutGroupString(context, /* default shortcut = */ true, /* non-standard shortcut = */ true));
    settings.setValue(nonStandardKey, shortcut.operator QVariant());   // Need this esoteric syntax to make compilers happy with Qt4
    settings.endGroup();

    // Need to emit the notification is there's no user shortcut overriding the default one
    settings.beginGroup(shortcutGroupString(context, /* default shortcut = */ false, /* non-standard shortcut = */ true));
    QVariant userShortcut = settings.value(nonStandardKey);
    settings.endGroup();

    if (!userShortcut.isValid() || ((userShortcut.type() != QVariant::KeySequence) && !userShortcut.canConvert(QVariant::KeySequence))) {
        QNTRACE("Found no user shortcut overriding the default one");
        emit nonStandardShortcutChanged(nonStandardKey, shortcut, context);
    }
}

QKeySequence ShortcutManagerPrivate::defaultShortcut(const QKeySequence::StandardKey standardKey,
                                                     const QString & standardKeyString, const QString & context) const
{
    QNDEBUG("ShortcutManagerPrivate::defaultShortcut: standard key = " << standardKeyString << " (" << standardKey
            << "), context = " << context);

    ApplicationSettings settings;

    settings.beginGroup(shortcutGroupString(context, /* default shortcut = */ true, /* non-standard shortcut = */ false));
    QVariant value = settings.value(standardKeyString);
    settings.endGroup();

    if (!value.isValid() || ((value.type() != QVariant::KeySequence) && !value.canConvert(QVariant::KeySequence))) {
        QNTRACE("Can't find default shortcut in app settings, returning the platform-specific default from QKeySequence");
        return QKeySequence(standardKey);
    }

    return qvariant_cast<QKeySequence>(value);
}

QKeySequence ShortcutManagerPrivate::defaultShortcut(const QString & nonStandardKey, const QString & context) const
{
    QNDEBUG("ShortcutManagerPrivate::defaultShortcut: non-standard key = " << nonStandardKey << ", context = " << context);

    ApplicationSettings settings;

    settings.beginGroup(shortcutGroupString(context, /* default shortcut = */ true, /* non-standard shortcut = */ true));
    QVariant value = settings.value(nonStandardKey);
    settings.endGroup();

    if (!value.isValid() || ((value.type() != QVariant::KeySequence) && !value.canConvert(QVariant::KeySequence))) {
        QNTRACE("Can't find default shortcut in app settings, returning the platform-specific default (or invalid shortcut) from QKeySequence");
        return QKeySequence(nonStandardKey);
    }

    return qvariant_cast<QKeySequence>(value);
}

QString ShortcutManagerPrivate::standardKeyToString(const QKeySequence::StandardKey standardKey) const
{
    QNTRACE("ShortcutManagerPrivate::standardKeyToString: standard key = " << standardKey);

#define PRINT_ITEM(item) \
    case QKeySequence::item: return #item

    switch(standardKey)
    {
        PRINT_ITEM(AddTab);
        PRINT_ITEM(Back);
        PRINT_ITEM(Bold);
        PRINT_ITEM(Close);
        PRINT_ITEM(Copy);
        PRINT_ITEM(Cut);
        PRINT_ITEM(DeleteEndOfLine);
        PRINT_ITEM(DeleteEndOfWord);
        PRINT_ITEM(DeleteStartOfWord);
        PRINT_ITEM(Find);
        PRINT_ITEM(FindNext);
        PRINT_ITEM(FindPrevious);
        PRINT_ITEM(Forward);
        PRINT_ITEM(HelpContents);
        PRINT_ITEM(InsertLineSeparator);
        PRINT_ITEM(InsertParagraphSeparator);
        PRINT_ITEM(Italic);
        PRINT_ITEM(MoveToEndOfBlock);
        PRINT_ITEM(MoveToEndOfDocument);
        PRINT_ITEM(MoveToEndOfLine);
        PRINT_ITEM(MoveToNextChar);
        PRINT_ITEM(MoveToNextLine);
        PRINT_ITEM(MoveToNextPage);
        PRINT_ITEM(MoveToNextWord);
        PRINT_ITEM(MoveToPreviousChar);
        PRINT_ITEM(MoveToPreviousLine);
        PRINT_ITEM(MoveToPreviousPage);
        PRINT_ITEM(MoveToPreviousWord);
        PRINT_ITEM(MoveToStartOfBlock);
        PRINT_ITEM(MoveToStartOfDocument);
        PRINT_ITEM(MoveToStartOfLine);
        PRINT_ITEM(New);
        PRINT_ITEM(NextChild);
        PRINT_ITEM(Open);
        PRINT_ITEM(Paste);
        PRINT_ITEM(Preferences);
        PRINT_ITEM(PreviousChild);
        PRINT_ITEM(Print);
        PRINT_ITEM(Quit);
        PRINT_ITEM(Redo);
        PRINT_ITEM(Refresh);
        PRINT_ITEM(Replace);
        PRINT_ITEM(SaveAs);
        PRINT_ITEM(Save);
        PRINT_ITEM(SelectAll);
        PRINT_ITEM(SelectEndOfBlock);
        PRINT_ITEM(SelectEndOfDocument);
        PRINT_ITEM(SelectEndOfLine);
        PRINT_ITEM(SelectNextChar);
        PRINT_ITEM(SelectNextLine);
        PRINT_ITEM(SelectNextPage);
        PRINT_ITEM(SelectNextWord);
        PRINT_ITEM(SelectPreviousChar);
        PRINT_ITEM(SelectPreviousLine);
        PRINT_ITEM(SelectPreviousPage);
        PRINT_ITEM(SelectPreviousWord);
        PRINT_ITEM(SelectStartOfBlock);
        PRINT_ITEM(SelectStartOfDocument);
        PRINT_ITEM(SelectStartOfLine);
        PRINT_ITEM(Underline);
        PRINT_ITEM(Undo);
        PRINT_ITEM(UnknownKey);
        PRINT_ITEM(WhatsThis);
        PRINT_ITEM(ZoomIn);
        PRINT_ITEM(ZoomOut);
#if QT_VERSION >= 0x050000
        PRINT_ITEM(FullScreen);
        PRINT_ITEM(Backspace);
        PRINT_ITEM(DeleteCompleteLine);
        PRINT_ITEM(Delete);
        PRINT_ITEM(Deselect);
#endif
    default:
        {
            QNWARNING("Unknown standard key: " << standardKey);
            return QString();
        }
    }
}

QString ShortcutManagerPrivate::shortcutGroupString(const QString & context, const bool defaultShortcut, const bool nonStandardShortcut) const
{
    QNTRACE("ShortcutManagerPrivate::shortcutGroupString: context = " << context << ", default shortcut = "
            << (defaultShortcut ? "true" : "false") << ", non-standard shortcut = " << (nonStandardShortcut ? "true" : "false"));

    return (QString(defaultShortcut ? "DefaultShortcuts-" : "UserShortcuts-") +
            (context.isEmpty()
             ? QString(nonStandardShortcut ? "NonStandard" : "General")
             : context));
}

} // namespace qute_note
