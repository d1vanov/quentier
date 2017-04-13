#ifndef QUENTIER_DIALOGS_ENEX_EXPORT_DIALOG_H
#define QUENTIER_DIALOGS_ENEX_EXPORT_DIALOG_H

#include <quentier/utility/Macros.h>
#include <quentier/types/Account.h>
#include <QDialog>

namespace Ui {
class EnexExportDialog;
}

namespace quentier {

class EnexExportDialog : public QDialog
{
    Q_OBJECT
public:
    explicit EnexExportDialog(const Account & account, QWidget * parent = Q_NULLPTR,
                              const QString & suggestedFileName = QString());
    virtual ~EnexExportDialog();

    bool exportTags() const;
    QString exportEnexFilePath() const;

Q_SIGNALS:
    void exportTagsOptionChanged(bool checked);

private Q_SLOTS:
    void onExportTagsOptionChanged(int state);
    void onBrowseFolderButtonPressed();
    void onFileNameEdited(const QString & name);
    void onFolderEdited(const QString & name);

private:
    void createConnections();
    void persistExportFolderSetting();

    void setStatusText(const QString & text);
    void clearAndHideStatus();

private:
    Ui::EnexExportDialog *  m_pUi;
    Account                 m_currentAccount;
};

} // namespace quentier

#endif // QUENTIER_DIALOGS_ENEX_EXPORT_DIALOG_H
