// File menu
PROCESS_ACTION_SHORTCUT(NewNote, ShortcutManager::NewNote, "File");
PROCESS_ACTION_SHORTCUT(NewNotebook, ShortcutManager::NewNotebook, "File");
PROCESS_ACTION_SHORTCUT(NewTag, ShortcutManager::NewTag, "File");
PROCESS_ACTION_SHORTCUT(NewSavedSearch, ShortcutManager::NewSavedSearch, "File");
PROCESS_ACTION_SHORTCUT(Print, QKeySequence::Print, "File");
PROCESS_ACTION_SHORTCUT(Quit, QKeySequence::Quit, "File");

// Edit menu
PROCESS_ACTION_SHORTCUT(Undo, QKeySequence::Undo, "Edit");
PROCESS_ACTION_SHORTCUT(Redo, QKeySequence::Redo, "Edit");
PROCESS_ACTION_SHORTCUT(Cut, QKeySequence::Cut, "Edit");
PROCESS_ACTION_SHORTCUT(Copy, QKeySequence::Copy, "Edit");
PROCESS_ACTION_SHORTCUT(Paste, QKeySequence::Paste, "Edit");
PROCESS_ACTION_SHORTCUT(PasteUnformatted, ShortcutManager::PasteUnformatted, "Edit");
PROCESS_ACTION_SHORTCUT(SelectAll, QKeySequence::SelectAll, "Edit");
PROCESS_ACTION_SHORTCUT(Delete, QKeySequence::Delete, "Edit");
PROCESS_ACTION_SHORTCUT(SaveNote, QKeySequence::Save, "Edit");

// Find and replace menu
PROCESS_ACTION_SHORTCUT(FindNote, ShortcutManager::NoteSearch, "Edit");
PROCESS_ACTION_SHORTCUT(FindInsideNote, QKeySequence::Find, "Edit");
PROCESS_ACTION_SHORTCUT(FindNext, QKeySequence::FindNext, "Edit");
PROCESS_ACTION_SHORTCUT(FindPrevious, QKeySequence::FindPrevious, "Edit");
PROCESS_ACTION_SHORTCUT(ReplaceInNote, QKeySequence::Replace, "Edit");
PROCESS_ACTION_SHORTCUT(SpellCheck, ShortcutManager::SpellCheck, "Edit");

// View menu
PROCESS_NON_STANDARD_ACTION_SHORTCUT(Notes, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(Notebooks, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(Tags, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(SavedSearches, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(ShowSidePanel, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(ShowFavorites, "View");
PROCESS_ACTION_SHORTCUT(ShowNotebooks, ShortcutManager::ShowNotebooks, "View");
PROCESS_ACTION_SHORTCUT(ShowTags, ShortcutManager::ShowTags, "View");
PROCESS_ACTION_SHORTCUT(ShowSavedSearches, ShortcutManager::ShowSavedSearches, "View");
PROCESS_ACTION_SHORTCUT(ShowDeletedNotes, ShortcutManager::ShowDeletedNotes, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(ShowNotesList, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(ShowNoteEditor, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(ShowToolbar, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(ShowStatusBar, "View");

// Format menu
PROCESS_ACTION_SHORTCUT(FontBold, QKeySequence::Bold, "Format");
PROCESS_ACTION_SHORTCUT(FontUnderlined, QKeySequence::Underline, "Format");
PROCESS_ACTION_SHORTCUT(FontItalic, QKeySequence::Italic, "Format");
PROCESS_ACTION_SHORTCUT(FontStrikethrough, ShortcutManager::Strikethrough, "Format");
PROCESS_ACTION_SHORTCUT(FontHighlight, ShortcutManager::Highlight, "Format");
PROCESS_ACTION_SHORTCUT(FontUpperIndex, ShortcutManager::UpperIndex, "Format");
PROCESS_ACTION_SHORTCUT(FontLowerIndex, ShortcutManager::LowerIndex, "Format");
PROCESS_ACTION_SHORTCUT(IncreaseFontSize, ShortcutManager::IncreaseFontSize, "Format");
PROCESS_ACTION_SHORTCUT(DecreaseFontSize, ShortcutManager::DecreaseFontSize, "Format");
PROCESS_ACTION_SHORTCUT(InsertHorizontalLine, ShortcutManager::InsertHorizontalLine, "Format");

PROCESS_ACTION_SHORTCUT(InsertTable, ShortcutManager::InsertTable, "Format");
PROCESS_ACTION_SHORTCUT(InsertRow, ShortcutManager::InsertRow, "Format");
PROCESS_ACTION_SHORTCUT(InsertColumn, ShortcutManager::InsertColumn, "Format");
PROCESS_ACTION_SHORTCUT(RemoveRow, ShortcutManager::RemoveRow, "Format");
PROCESS_ACTION_SHORTCUT(RemoveColumn, ShortcutManager::RemoveColumn, "Format");

PROCESS_ACTION_SHORTCUT(AlignLeft, ShortcutManager::AlignLeft, "Format");
PROCESS_ACTION_SHORTCUT(AlignCenter, ShortcutManager::AlignCenter, "Format");
PROCESS_ACTION_SHORTCUT(AlignRight, ShortcutManager::AlignRight, "Format");
PROCESS_ACTION_SHORTCUT(AlignFull, ShortcutManager::AlignFull, "Format");

PROCESS_ACTION_SHORTCUT(InsertBulletedList, ShortcutManager::InsertBulletedList, "Format");
PROCESS_ACTION_SHORTCUT(InsertNumberedList, ShortcutManager::InsertNumberedList, "Format");

PROCESS_ACTION_SHORTCUT(IncreaseIndentation, ShortcutManager::IncreaseIndentation, "Format");
PROCESS_ACTION_SHORTCUT(DecreaseIndentation, ShortcutManager::DecreaseIndentation, "Format");

PROCESS_ACTION_SHORTCUT(EditHyperlink, ShortcutManager::EditHyperlink, "Format");
PROCESS_ACTION_SHORTCUT(CopyHyperlink, ShortcutManager::CopyHyperlink, "Format");
PROCESS_ACTION_SHORTCUT(RemoveHyperlink, ShortcutManager::RemoveHyperlink, "Format");

// Service menu
PROCESS_ACTION_SHORTCUT(Synchronize, ShortcutManager::Synchronize, "Service");
PROCESS_ACTION_SHORTCUT(AccountInfo, ShortcutManager::AccountInfo, "Service");
PROCESS_ACTION_SHORTCUT(LocalStorageStatus, ShortcutManager::LocalStorageStatus, "Service");
PROCESS_ACTION_SHORTCUT(ImportFolders, ShortcutManager::ImportFolders, "Service");

// General menu
PROCESS_ACTION_SHORTCUT(Preferences, QKeySequence::Preferences, "General");

// Help menu
PROCESS_ACTION_SHORTCUT(ReleaseNotes, ShortcutManager::ReleaseNotes, "Help");
PROCESS_ACTION_SHORTCUT(ViewLogs, ShortcutManager::ViewLogs, "Help");
PROCESS_ACTION_SHORTCUT(About, ShortcutManager::About, "Help");
