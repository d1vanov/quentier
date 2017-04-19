#include "EnexImportDialog.h"
#include "ui_EnexImportDialog.h"
#include "../models/NotebookModel.h"
#include "../SettingsNames.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/DesktopServices.h>
#include <quentier/types/Notebook.h>
#include <QStringListModel>
#include <QModelIndex>
#include <QCompleter>
#include <QFileInfo>
#include <QScopedPointer>
#include <QFileDialog>
#include <algorithm>

namespace quentier {

EnexImportDialog::EnexImportDialog(const Account & account,
                                   NotebookModel & notebookModel,
                                   QWidget *parent) :
    QDialog(parent),
    m_pUi(new Ui::EnexImportDialog),
    m_currentAccount(account),
    m_pNotebookModel(&notebookModel),
    m_pNotebookNamesModel(new QStringListModel(this))
{
    m_pUi->setupUi(this);

    setWindowTitle(tr("Import ENEX"));

    fillNotebookNames();
    m_pUi->notebookNameComboBox->setModel(m_pNotebookNamesModel);

    QCompleter * pCompleter = m_pUi->notebookNameComboBox->completer();
    if (pCompleter) {
        pCompleter->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    }

    fillDialogContents();
    createConnections();
}

EnexImportDialog::~EnexImportDialog()
{
    delete m_pUi;
}

QString EnexImportDialog::importEnexFilePath(ErrorString * pErrorDescription) const
{
    QNDEBUG(QStringLiteral("EnexImportDialog::importEnexFilePath"));

    QString currentFilePath = QDir::fromNativeSeparators(m_pUi->filePathLineEdit->text());
    QNTRACE(QStringLiteral("Current file path: ") << currentFilePath);

    if (currentFilePath.isEmpty()) {
        return QString();
    }

    QFileInfo fileInfo(currentFilePath);
    if (!fileInfo.exists())
    {
        QNDEBUG(QStringLiteral("ENEX file at specified path doesn't exist"));
        if (pErrorDescription) {
            pErrorDescription->base() = QT_TRANSLATE_NOOP("", "ENEX file at specified path doesn't exist");
        }

        return QString();
    }

    if (!fileInfo.isFile())
    {
        QNDEBUG(QStringLiteral("The specified path is not a file"));
        if (pErrorDescription) {
            pErrorDescription->base() = QT_TRANSLATE_NOOP("", "The specified path is not a file");
        }

        return QString();
    }

    if (!fileInfo.isReadable())
    {
        QNDEBUG(QStringLiteral("The specified file is not readable"));
        if (pErrorDescription) {
            pErrorDescription->base() = QT_TRANSLATE_NOOP("", "The specified file is not readable");
        }

        return QString();
    }

    return currentFilePath;
}

QString EnexImportDialog::notebookName(ErrorString * pErrorDescription) const
{
    QString currentNotebookName = m_pUi->notebookNameComboBox->currentText();
    if (currentNotebookName.isEmpty()) {
        return QString();
    }

    if (Notebook::validateName(currentNotebookName, pErrorDescription)) {
        return currentNotebookName;
    }

    return QString();
}

void EnexImportDialog::onBrowsePushButtonClicked()
{
    QNDEBUG(QStringLiteral("EnexImportDialog::onBrowsePushButtonClicked"));

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_AUXILIARY_SETTINGS);
    appSettings.beginGroup(ENEX_EXPORT_IMPORT_SETTINGS_GROUP_NAME);
    QString lastEnexImportPath = appSettings.value(LAST_IMPORT_ENEX_PATH_SETTINGS_KEY).toString();
    appSettings.endGroup();

    if (lastEnexImportPath.isEmpty()) {
        lastEnexImportPath = documentsPath();
    }

    QScopedPointer<QFileDialog> pEnexFileDialog(new QFileDialog(this,
                                                                tr("Please select the ENEX file to import"),
                                                                lastEnexImportPath));
    pEnexFileDialog->setWindowModality(Qt::WindowModal);
    pEnexFileDialog->setAcceptMode(QFileDialog::AcceptOpen);
    pEnexFileDialog->setFileMode(QFileDialog::ExistingFile);
    pEnexFileDialog->setDefaultSuffix(QStringLiteral("enex"));

    if (pEnexFileDialog->exec() != QDialog::Accepted) {
        QNDEBUG(QStringLiteral("The import of ENEX was cancelled"));
        return;
    }

    QStringList selectedFiles = pEnexFileDialog->selectedFiles();
    int numSelectedFiles = selectedFiles.size();

    if (numSelectedFiles == 0) {
        QNDEBUG(QStringLiteral("No ENEX file was selected"));
        setStatusText(tr("No ENEX file was selected"));
        return;
    }

    if (numSelectedFiles > 1) {
        QNDEBUG(QStringLiteral("More than one ENEX files were selected"));
        setStatusText(tr("More than one ENEX files were selected"));
        return;
    }

    QFileInfo enexFileInfo(selectedFiles[0]);
    if (!enexFileInfo.exists()) {
        QNDEBUG(QStringLiteral("The selected ENEX file does not exist"));
        setStatusText(tr("The selected ENEX file does not exist"));
        return;
    }

    if (!enexFileInfo.isReadable()) {
        QNDEBUG(QStringLiteral("The selected ENEX file is not readable"));
        setStatusText("The selected ENEX file is not readable");
        return;
    }

    lastEnexImportPath = pEnexFileDialog->directory().absolutePath();

    if (!lastEnexImportPath.isEmpty()) {
        appSettings.beginGroup(ENEX_EXPORT_IMPORT_SETTINGS_GROUP_NAME);
        appSettings.setValue(LAST_IMPORT_ENEX_PATH_SETTINGS_KEY, lastEnexImportPath);
        appSettings.endGroup();
    }

    m_pUi->filePathLineEdit->setText(QDir::toNativeSeparators(enexFileInfo.absoluteFilePath()));
    checkConditionsAndEnableDisableOkButton();
}

void EnexImportDialog::onNotebookNameEdited(const QString & name)
{
    QNDEBUG(QStringLiteral("EnexImportDialog::onNotebookNameEdited: ") << name);
    checkConditionsAndEnableDisableOkButton();
}

void EnexImportDialog::onEnexFilePathEdited(const QString & path)
{
    QNDEBUG(QStringLiteral("EnexImportDialog::onEnexFilePathEdited: ") << path);
    checkConditionsAndEnableDisableOkButton();
}

void EnexImportDialog::dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
                                 )
#else
                                , const QVector<int> & roles)
#endif
{
    QNDEBUG(QStringLiteral("EnexImportDialog::dataChanged"));

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    Q_UNUSED(roles)
#endif

    if (!topLeft.isValid() || !bottomRight.isValid()) {
        QNDEBUG(QStringLiteral("At least one of changed indexes is invalid"));
        fillNotebookNames();
        return;
    }

    if ((topLeft.column() > NotebookModel::Columns::Name) ||
        (bottomRight.column() < NotebookModel::Columns::Name))
    {
        QNTRACE(QStringLiteral("The updated indexed don't contain the notebook name"));
        return;
    }

    fillNotebookNames();
}

void EnexImportDialog::rowsInserted(const QModelIndex & parent, int start, int end)
{
    QNDEBUG(QStringLiteral("EnexImportDialog::rowsInserted"));

    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)

    fillNotebookNames();
}

void EnexImportDialog::rowsAboutToBeRemoved(const QModelIndex & parent, int start, int end)
{
    QNDEBUG(QStringLiteral("EnexImportDialog::rowsAboutToBeRemoved"));

    if (Q_UNLIKELY(m_pNotebookModel.isNull())) {
        QNDEBUG(QStringLiteral("Notebook model is null, nothing to do"));
        return;
    }

    QStringList currentNotebookNames = m_pNotebookNamesModel->stringList();

    for(int i = start; i <= end; ++i)
    {
        QModelIndex index = m_pNotebookModel->index(i, NotebookModel::Columns::Name, parent);
        QString removedNotebookName = m_pNotebookModel->data(index).toString();
        if (Q_UNLIKELY(removedNotebookName.isEmpty())) {
            continue;
        }

        auto it = std::lower_bound(currentNotebookNames.constBegin(),
                                   currentNotebookNames.constEnd(),
                                   removedNotebookName);
        if ((it != currentNotebookNames.constEnd()) && (*it == removedNotebookName)) {
            int offset = static_cast<int>(std::distance(currentNotebookNames.constBegin(), it));
            QStringList::iterator nit = currentNotebookNames.begin() + offset;
            Q_UNUSED(currentNotebookNames.erase(nit));
        }
    }

    m_pNotebookNamesModel->setStringList(currentNotebookNames);
}

void EnexImportDialog::createConnections()
{
    QNDEBUG(QStringLiteral("EnexImportDialog::createConnections"));

    QObject::connect(m_pUi->browsePushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(EnexImportDialog,onBrowsePushButtonClicked));
    QObject::connect(m_pUi->filePathLineEdit, QNSIGNAL(QLineEdit,textEdited,QString),
                     this, QNSLOT(EnexImportDialog,onEnexFilePathEdited,QString));
    QObject::connect(m_pUi->notebookNameComboBox, QNSIGNAL(QComboBox,editTextChanged,QString),
                     this, QNSLOT(EnexImportDialog,onNotebookNameEdited,QString));
    QObject::connect(m_pUi->notebookNameComboBox, SIGNAL(currentIndexChanged(QString)),
                     this, SLOT(onNotebookNameEdited(QString)));

    if (!m_pNotebookModel.isNull())
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        QObject::connect(m_pNotebookModel.data(), &NotebookModel::dataChanged,
                         this, &EnexImportDialog::dataChanged);
#else
        QObject::connect(m_pNotebookModel.data(), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                         this, SLOT(dataChanged(QModelIndex,QModelIndex)));
#endif
        QObject::connect(m_pNotebookModel.data(), QNSIGNAL(NotebookModel,rowsInserted,QModelIndex,int,int),
                         this, QNSLOT(EnexImportDialog,rowsInserted,QModelIndex,int,int));
        QObject::connect(m_pNotebookModel.data(), QNSIGNAL(NotebookModel,rowsAboutToBeRemoved,QModelIndex,int,int),
                         this, QNSLOT(EnexImportDialog,rowsAboutToBeRemoved,QModelIndex,int,int));
    }
}

void EnexImportDialog::fillNotebookNames()
{
    QNDEBUG(QStringLiteral("EnexImportDialog::fillNotebookNames"));

    QStringList notebookNames;

    if (!m_pNotebookModel.isNull()) {
        NotebookModel::NotebookFilters filter(NotebookModel::NotebookFilter::CanCreateNotes);
        notebookNames = m_pNotebookModel->notebookNames(filter);
    }

    m_pNotebookNamesModel->setStringList(notebookNames);
}

void EnexImportDialog::fillDialogContents()
{
    QNDEBUG(QStringLiteral("EnexImportDialog::fillDialogContents"));

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_AUXILIARY_SETTINGS);
    appSettings.beginGroup(ENEX_EXPORT_IMPORT_SETTINGS_GROUP_NAME);
    QString lastImportEnexNotebookName = appSettings.value(LAST_IMPORT_ENEX_NOTEBOOK_NAME_SETTINGS_KEY).toString();
    appSettings.endGroup();

    if (lastImportEnexNotebookName.isEmpty()) {
        lastImportEnexNotebookName = tr("Imported notes");
    }

    m_pUi->notebookNameComboBox->setCurrentText(lastImportEnexNotebookName);

    m_pUi->statusTextLabel->setHidden(true);
}

void EnexImportDialog::setStatusText(const QString & text)
{
    m_pUi->statusTextLabel->setText(text);
    m_pUi->statusTextLabel->setHidden(false);
}

void EnexImportDialog::clearAndHideStatus()
{
    QNDEBUG(QStringLiteral("EnexImportDialog::clearAndHideStatus"));

    m_pUi->statusTextLabel->setText(QString());
    m_pUi->statusTextLabel->setHidden(true);
}

void EnexImportDialog::checkConditionsAndEnableDisableOkButton()
{
    QNDEBUG(QStringLiteral("EnexImportDialog::checkConditionsAndEnableDisableOkButton"));

    ErrorString error;
    QString enexFilePath = importEnexFilePath(&error);
    if (enexFilePath.isEmpty()) {
        QNDEBUG(QStringLiteral("The enex file path is invalid, disabling the ok button"));
        m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        setStatusText(error.localizedString());
        return;
    }

    QString currentNotebookName = notebookName(&error);
    if (currentNotebookName.isEmpty()) {
        QNDEBUG(QStringLiteral("Notebook name is not set or is not valid, "
                               "disabling the ok button"));
        m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        setStatusText(error.localizedString());
        return;
    }

    m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(false);
    clearAndHideStatus();
}

} // namespace quentier
