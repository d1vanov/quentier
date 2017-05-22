#ifndef QUENTIER_DIALOGS_PREFERENCES_DIALOG_H
#define QUENTIER_DIALOGS_PREFERENCES_DIALOG_H

#include <quentier/utility/Macros.h>
#include <QDialog>

namespace Ui {
class PreferencesDialog;
}

QT_FORWARD_DECLARE_CLASS(QStringListModel)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(AccountManager)
QT_FORWARD_DECLARE_CLASS(SystemTrayIconManager)

class PreferencesDialog: public QDialog
{
    Q_OBJECT
public:
    explicit PreferencesDialog(AccountManager & accountManager,
                               SystemTrayIconManager & systemTrayIconManager,
                               QWidget * parent = Q_NULLPTR);
    virtual ~PreferencesDialog();

Q_SIGNALS:
    void noteEditorUseLimitedFontsOptionChanged(bool enabled);
    void synchronizationDownloadNoteThumbnailsOptionChanged(bool enabled);
    void synchronizationNoteThumbnailsStoragePathChanged(QString path);

private Q_SLOTS:
    // System tray tab
    void onShowSystemTrayIconCheckboxToggled(bool checked);
    void onCloseToSystemTrayCheckboxToggled(bool checked);
    void onMinimizeToSystemTrayCheckboxToggled(bool checked);
    void onStartMinimizedToSystemTrayCheckboxToggled(bool checked);

    void onSingleClickTrayActionChanged(int action);
    void onMiddleClickTrayActionChanged(int action);
    void onDoubleClickTrayActionChanged(int action);

    // Note editor tab
    void onNoteEditorUseLimitedFontsCheckboxToggled(bool checked);

    // Synchronization tab
    void onDownloadNoteThumbnailsCheckboxToggled(bool checked);
    void onNoteThumbnailsStoragePathChanged();

private:
    void setupCurrentSettingsState();
    void createConnections();

private:
    Ui::PreferencesDialog *         m_pUi;
    AccountManager &                m_accountManager;
    SystemTrayIconManager &         m_systemTrayIconManager;
    QStringListModel *              m_pTrayActionsModel;
};

} // namespace quentier

#endif // QUENTIER_DIALOGS_PREFERENCES_DIALOG_H
