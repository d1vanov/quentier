#include "EnexExportDialog.h"
#include "ui_EnexExportDialog.h"
#include "../SettingsNames.h"
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/StandardPaths.h>
#include <quentier/logging/QuentierLogger.h>
#include <QFileInfo>
#include <QScopedPointer>
#include <QFileDialog>
#include <QDir>

namespace quentier {

EnexExportDialog::EnexExportDialog(const Account & account,
                                   QWidget * parent,
                                   const QString & suggestedFileName) :
    QDialog(parent),
    m_pUi(new Ui::EnexExportDialog),
    m_currentAccount(account)
{
    m_pUi->setupUi(this);

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_AUXILIARY_SETTINGS);
    appSettings.beginGroup(ENEX_EXPORT_IMPORT_SETTINGS_GROUP_NAME);
    QString lastEnexExportPath = appSettings.value(LAST_EXPORT_NOTE_TO_ENEX_PATH_SETTINGS_KEY).toString();
    bool exportTags = (appSettings.contains(LAST_EXPORT_NOTE_TO_ENEX_EXPORT_TAGS_SETTINGS_KEY)
                       ? appSettings.value(LAST_EXPORT_NOTE_TO_ENEX_EXPORT_TAGS_SETTINGS_KEY).toBool()
                       : true);
    appSettings.endGroup();

    if (lastEnexExportPath.isEmpty()) {
        lastEnexExportPath = documentsPath();
    }

    QFileInfo lastEnexExportPathInfo(lastEnexExportPath);
    if (lastEnexExportPathInfo.exists() && lastEnexExportPathInfo.isDir() && lastEnexExportPathInfo.isWritable()) {
        m_pUi->folderLineEdit->setText(lastEnexExportPath);
        QNDEBUG(QStringLiteral("EnexExportDialog: initialized folder path to ") << lastEnexExportPath);
    }

    m_pUi->exportTagsCheckBox->setChecked(exportTags);
    m_pUi->fileNameLineEdit->setText(suggestedFileName);
    m_pUi->statusTextLabel->setHidden(true);

    createConnections();
    checkConditionsAndEnableDisableOkButton();
}

EnexExportDialog::~EnexExportDialog()
{
    delete m_pUi;
}

bool EnexExportDialog::exportTags() const
{
    return m_pUi->exportTagsCheckBox->isChecked();
}

QString EnexExportDialog::exportEnexFilePath() const
{
    QString folderPath = m_pUi->folderLineEdit->text();
    QString convertedFolderPath = QDir::fromNativeSeparators(folderPath);

    QString fileName = m_pUi->fileNameLineEdit->text();

    QNDEBUG(QStringLiteral("EnexExportDialog::exportEnexFilePath: folder path = ")
            << folderPath << QStringLiteral(", converted folder path = ") << convertedFolderPath
            << QStringLiteral(", file name = ") << fileName);

    if (convertedFolderPath.isEmpty()) {
        QNDEBUG(QStringLiteral("No folder path was specified"));
        return QString();
    }

    if (fileName.isEmpty()) {
        QNDEBUG(QStringLiteral("No file name was specified"));
        return QString();
    }

    QFileInfo dirInfo(convertedFolderPath);
    if (!dirInfo.exists()) {
        QNDEBUG(QStringLiteral("Directory doesn't exist"));
        return QString();
    }

    if (!dirInfo.isDir()) {
        QNDEBUG(QStringLiteral("Non-directory was selected"));
        return QString();
    }

    if (!dirInfo.isWritable()) {
        QNDEBUG(QStringLiteral("Non-writable directory was selected"));
        return QString();
    }

    if (!convertedFolderPath.endsWith(QStringLiteral("/"))) {
        convertedFolderPath += QStringLiteral("/");
    }

    if (!fileName.endsWith(QStringLiteral(".enex"), Qt::CaseInsensitive)) {
        fileName += QStringLiteral(".enex");
    }

    convertedFolderPath += fileName;
    QNDEBUG(QStringLiteral("Absolute ENEX export file path: ") << convertedFolderPath);
    return convertedFolderPath;
}

void EnexExportDialog::onExportTagsOptionChanged(int state)
{
    QNDEBUG(QStringLiteral("EnexExportDialog::onExportTagsOptionChanged: state = ") << state);

    bool checked = (state == Qt::Checked);

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_AUXILIARY_SETTINGS);
    appSettings.beginGroup(ENEX_EXPORT_IMPORT_SETTINGS_GROUP_NAME);
    appSettings.setValue(LAST_EXPORT_NOTE_TO_ENEX_EXPORT_TAGS_SETTINGS_KEY, checked);
    appSettings.endGroup();

    emit exportTagsOptionChanged(checked);
}

void EnexExportDialog::onBrowseFolderButtonPressed()
{
    QNDEBUG(QStringLiteral("EnexExportDialog::onBrowseFolderButtonPressed"));

    clearAndHideStatus();

    QString currentExportDir = m_pUi->folderLineEdit->text();
    if (!currentExportDir.isEmpty())
    {
        QFileInfo currentExportDirInfo(currentExportDir);
        if (!currentExportDirInfo.exists() || !currentExportDirInfo.isDir()) {
            currentExportDir.clear();
        }
    }

    QScopedPointer<QFileDialog> pFileDialog(new QFileDialog(this,
                                                            tr("Please select the folder in which to store the output ENEX file"),
                                                            currentExportDir));
    pFileDialog->setWindowModality(Qt::WindowModal);
    pFileDialog->setAcceptMode(QFileDialog::AcceptSave);
    pFileDialog->setFileMode(QFileDialog::DirectoryOnly);
    pFileDialog->setOption(QFileDialog::ShowDirsOnly, true);

    if (pFileDialog->exec() != QDialog::Accepted) {
        QNDEBUG(QStringLiteral("Cancelled changing the ENEX file export dir"));
        return;
    }

    QStringList dirs = pFileDialog->selectedFiles();
    if (dirs.isEmpty()) {
        QNDEBUG(QStringLiteral("No directories were selected"));
        setStatusText(tr("No directory for ENEX export was selected"));
        return;
    }

    QString dir = dirs.at(0);

    QFileInfo dirInfo(dir);
    if (!dirInfo.exists()) {
        QNDEBUG(QStringLiteral("Nonexistent directory or file was selected: ") << dir);
        setStatusText(tr("No existing directory was selected"));
        return;
    }

    if (!dirInfo.isDir()) {
        QNDEBUG(QStringLiteral("Non-directory was selected: ") << dir);
        setStatusText(tr("Non-directory was selected"));
        return;
    }

    if (!dirInfo.isWritable()) {
        QNDEBUG(QStringLiteral("Non-writable directory was selected: ") << dir);
        setStatusText(tr("The selected directory is not writable"));
        return;
    }

    m_pUi->folderLineEdit->setText(QDir::toNativeSeparators(dirInfo.absoluteFilePath()));
    persistExportFolderSetting();
    checkConditionsAndEnableDisableOkButton();
}

void EnexExportDialog::onFileNameEdited(const QString & name)
{
    QNDEBUG(QStringLiteral("EnexExportDialog::onFileNameEdited: ") << name);
    clearAndHideStatus();
    checkConditionsAndEnableDisableOkButton();
}

void EnexExportDialog::onFolderEdited(const QString & name)
{
    QNDEBUG(QStringLiteral("EnexExportDialog::onFolderEdited: ") << name);
    clearAndHideStatus();
    checkConditionsAndEnableDisableOkButton();
}

void EnexExportDialog::createConnections()
{
    QNDEBUG(QStringLiteral("EnexExportDialog::createConnections"));

    QObject::connect(m_pUi->exportTagsCheckBox, QNSIGNAL(QCheckBox,stateChanged,int),
                     this, QNSLOT(EnexExportDialog,onExportTagsOptionChanged,int));
    QObject::connect(m_pUi->browseFolderPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(EnexExportDialog,onBrowseFolderButtonPressed));
    QObject::connect(m_pUi->fileNameLineEdit, QNSIGNAL(QLineEdit,textEdited,QString),
                     this, QNSLOT(EnexExportDialog,onFileNameEdited,QString));
    QObject::connect(m_pUi->folderLineEdit, QNSIGNAL(QLineEdit,textEdited,QString),
                     this, QNSLOT(EnexExportDialog,onFolderEdited,QString));
}

void EnexExportDialog::persistExportFolderSetting()
{
    QString path = m_pUi->folderLineEdit->text();
    QString convertedPath = QDir::fromNativeSeparators(path);

    QNDEBUG(QStringLiteral("EnexExportDialog::persistExportFolderSetting: path = ")
            << path << QStringLiteral(", converted path: ") << convertedPath);

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_AUXILIARY_SETTINGS);
    appSettings.beginGroup(ENEX_EXPORT_IMPORT_SETTINGS_GROUP_NAME);
    appSettings.setValue(LAST_EXPORT_NOTE_TO_ENEX_PATH_SETTINGS_KEY, convertedPath);
    appSettings.endGroup();
}

void EnexExportDialog::checkConditionsAndEnableDisableOkButton()
{
    QNDEBUG(QStringLiteral("EnexExportDialog::checkConditionsAndEnableDisableOkButton"));

    QString fullFilePath = exportEnexFilePath();
    if (!fullFilePath.isEmpty()) {
        QNDEBUG(QStringLiteral("Full file path is not empty which means it has been "
                               "validated already, enabling the ok button"));
        m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        clearAndHideStatus();
        return;
    }

    m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);

    QString fileName = m_pUi->fileNameLineEdit->text();
    if (fileName.isEmpty()) {
        QNDEBUG(QStringLiteral("The file name is not set"));
        return;
    }

    QString folderPath = m_pUi->folderLineEdit->text();
    if (folderPath.isEmpty()) {
        QNDEBUG(QStringLiteral("The folder is not set"));
        return;
    }

    // If both are non-empty, they don't form a valid writable file path
    folderPath = QDir::fromNativeSeparators(folderPath);

    QFileInfo dirInfo(folderPath);
    if (!dirInfo.exists()) {
        QNDEBUG(QStringLiteral("Dir doesn't exist"));
        setStatusText(tr("The selected directory doens't exist"));
        return;
    }

    if (!dirInfo.isDir()) {
        QNDEBUG(QStringLiteral("Non-dir selected"));
        setStatusText(tr("The selected directory path is not a directory"));
        return;
    }

    if (!dirInfo.isWritable()) {
        QNDEBUG(QStringLiteral("Non-writable dir selected"));
        setStatusText(tr("The selected directory is not writable"));
        return;
    }
}

void EnexExportDialog::setStatusText(const QString & text)
{
    m_pUi->statusTextLabel->setText(text);
    m_pUi->statusTextLabel->setHidden(false);
}

void EnexExportDialog::clearAndHideStatus()
{
    QNDEBUG(QStringLiteral("EnexExportDialog::clearAndHideStatus"));

    m_pUi->statusTextLabel->setText(QString());
    m_pUi->statusTextLabel->setHidden(true);
}

} // namespace quentier
