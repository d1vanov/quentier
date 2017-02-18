/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ManageAccountsDialog.h"
#include "ui_ManageAccountsDialog.h"
#include "AddAccountDialog.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/StringUtils.h>
#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include <QRegExp>
#include <QToolTip>

#define NUM_ACCOUNTS_MODEL_COLUMNS (3)

namespace quentier {

class ManageAccountsDialog::AccountsModel: public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit AccountsModel(const QSharedPointer<QVector<Account> > & pAvailableAccounts,
                           QObject * parent = Q_NULLPTR);

    struct Columns
    {
        enum type
        {
            Type = 0,
            Username,
            DisplayName
        };
    };

    void beginUpdateAccounts();
    void endUpdateAccounts();

Q_SIGNALS:
    void badAccountDisplayName(ErrorString errorDescription, int row);
    void accountDisplayNameChanged(Account account);

private:
    virtual Qt::ItemFlags flags(const QModelIndex & index) const Q_DECL_OVERRIDE;
    virtual int rowCount(const QModelIndex & parent) const Q_DECL_OVERRIDE;
    virtual int columnCount(const QModelIndex & parent) const Q_DECL_OVERRIDE;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role) Q_DECL_OVERRIDE;

private:
    QSharedPointer<QVector<Account> >   m_pAvailableAccounts;
    StringUtils                         m_stringUtils;
};

class ManageAccountsDialog::AccountsModelDelegate: public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit AccountsModelDelegate(QObject * parent = Q_NULLPTR);

    virtual QWidget * createEditor(QWidget * parent,
                                   const QStyleOptionViewItem & option,
                                   const QModelIndex & index) const Q_DECL_OVERRIDE;
};

ManageAccountsDialog::ManageAccountsDialog(const QVector<Account> & availableAccounts,
                                           const int currentAccountRow, QWidget * parent) :
    QDialog(parent),
    m_pUi(new Ui::ManageAccountsDialog),
    m_pAvailableAccounts(new QVector<Account>(availableAccounts)),
    m_pAccountsModel(new AccountsModel(m_pAvailableAccounts))
{
    m_pUi->setupUi(this);
    setWindowTitle(tr("Manage accounts"));

    m_pUi->tableView->setModel(m_pAccountsModel.data());
    AccountsModelDelegate * pDelegate = new AccountsModelDelegate(m_pAccountsModel.data());
    m_pUi->tableView->setItemDelegate(pDelegate);

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    m_pUi->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
#else
    m_pUi->tableView->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
#endif

    int numAvailableAccounts = m_pAvailableAccounts->size();
    if ((currentAccountRow >= 0) && (currentAccountRow < numAvailableAccounts)) {
        QModelIndex index = m_pAccountsModel->index(currentAccountRow, AccountsModel::Columns::Username);
        m_pUi->tableView->setCurrentIndex(index);
    }

    QObject::connect(m_pAccountsModel.data(), QNSIGNAL(AccountsModel,badAccountDisplayName,ErrorString,int),
                     this, QNSLOT(ManageAccountsDialog,onBadAccountDisplayNameEntered,ErrorString,int));
    QObject::connect(m_pAccountsModel.data(), QNSIGNAL(AccountsModel,accountDisplayNameChanged,Account),
                     this, QNSIGNAL(ManageAccountsDialog,accountDisplayNameChanged,Account));
    QObject::connect(m_pUi->addNewAccountButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(ManageAccountsDialog,onAddAccountButtonPressed));
    QObject::connect(m_pUi->revokeAuthenticationButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(ManageAccountsDialog,onRevokeAuthenticationButtonPressed));
}

ManageAccountsDialog::~ManageAccountsDialog()
{
    delete m_pUi;
}

void ManageAccountsDialog::onAvailableAccountsChanged(const QVector<Account> & availableAccounts)
{
    QNDEBUG(QStringLiteral("ManageAccountsDialog::onAvailableAccountsChanged"));

    int newCurrentRow = -1;

    QModelIndex currentIndex = m_pUi->tableView->currentIndex();
    if (currentIndex.isValid())
    {
        int currentRow = currentIndex.row();
        if ((currentRow >= 0) && (currentRow <= m_pAvailableAccounts->size()))
        {
            const Account & currentAccount = (*m_pAvailableAccounts)[currentRow];
            newCurrentRow = availableAccounts.indexOf(currentAccount);
        }
    }

    m_pAccountsModel->beginUpdateAccounts();
    *m_pAvailableAccounts = availableAccounts;
    m_pAccountsModel->endUpdateAccounts();

    int numAvailableAccounts = m_pAvailableAccounts->size();
    if ((newCurrentRow >= 0) && (newCurrentRow < numAvailableAccounts)) {
        QModelIndex index = m_pAccountsModel->index(newCurrentRow, AccountsModel::Columns::Username);
        m_pUi->tableView->setCurrentIndex(index);
    }
}

void ManageAccountsDialog::onAddAccountButtonPressed()
{
    QNDEBUG(QStringLiteral("ManageAccountsDialog::onAddAccountButtonPressed"));

    QScopedPointer<AddAccountDialog> addAccountDialog(new AddAccountDialog(*m_pAvailableAccounts, this));
    addAccountDialog->setWindowModality(Qt::WindowModal);
    QObject::connect(addAccountDialog.data(), QNSIGNAL(AddAccountDialog,evernoteAccountAdditionRequested,QString),
                     this, QNSIGNAL(ManageAccountsDialog,evernoteAccountAdditionRequested,QString));
    QObject::connect(addAccountDialog.data(), QNSIGNAL(AddAccountDialog,localAccountAdditionRequested,QString,QString),
                     this, QNSIGNAL(ManageAccountsDialog,localAccountAdditionRequested,QString,QString));
    Q_UNUSED(addAccountDialog->exec())
}

void ManageAccountsDialog::onRevokeAuthenticationButtonPressed()
{
    QNDEBUG(QStringLiteral("ManageAccountsDialog::onRevokeAuthenticationButtonPressed"));

    QModelIndex currentIndex = m_pUi->tableView->currentIndex();
    if (Q_UNLIKELY(!currentIndex.isValid())) {
        QNTRACE(QStringLiteral("Current index is invalid"));
        return;
    }

    int currentRow = currentIndex.row();
    if (Q_UNLIKELY(currentRow < 0)) {
        QNTRACE(QStringLiteral("Current row is negative"));
        return;
    }

    if (Q_UNLIKELY(currentRow >= m_pAvailableAccounts->size())) {
        QNTRACE(QStringLiteral("Current row is larger than the number of available accounts"));
        return;
    }

    const Account & availableAccount = m_pAvailableAccounts->at(currentRow);
    if (availableAccount.type() == Account::Type::Local) {
        QNTRACE(QStringLiteral("The current account is local, nothing to do"));
        return;
    }

    emit revokeAuthentication(availableAccount.id());
}

void ManageAccountsDialog::onBadAccountDisplayNameEntered(ErrorString errorDescription, int row)
{
    QNINFO(QStringLiteral("Bad account display name entered: ")
           << errorDescription << QStringLiteral("; row = ") << row);

    QModelIndex index = m_pAccountsModel->index(row, AccountsModel::Columns::DisplayName);
    QRect rect = m_pUi->tableView->visualRect(index);
    QPoint pos = m_pUi->tableView->mapToGlobal(rect.bottomLeft());
    QToolTip::showText(pos, errorDescription.localizedString(), m_pUi->tableView);
}

ManageAccountsDialog::AccountsModel::AccountsModel(const QSharedPointer<QVector<Account> > & pAvailableAccounts,
                                                   QObject * parent) :
    QAbstractTableModel(parent),
    m_pAvailableAccounts(pAvailableAccounts),
    m_stringUtils()
{}

void ManageAccountsDialog::AccountsModel::beginUpdateAccounts()
{
    emit layoutAboutToBeChanged();
}

void ManageAccountsDialog::AccountsModel::endUpdateAccounts()
{
    emit layoutChanged();
}

Qt::ItemFlags ManageAccountsDialog::AccountsModel::flags(const QModelIndex & index) const
{
    Qt::ItemFlags indexFlags = QAbstractTableModel::flags(index);
    if (!index.isValid()) {
        return indexFlags;
    }

    int row = index.row();
    int column = index.column();

    if ((row < 0) || (row >= m_pAvailableAccounts->size()) ||
        (column < 0) || (column >= NUM_ACCOUNTS_MODEL_COLUMNS))
    {
        return indexFlags;
    }

    indexFlags |= Qt::ItemIsSelectable;
    indexFlags |= Qt::ItemIsEnabled;

    if (column == AccountsModel::Columns::DisplayName) {
        indexFlags |= Qt::ItemIsEditable;
    }

    return indexFlags;
}

int ManageAccountsDialog::AccountsModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_pAvailableAccounts->size();
}

int ManageAccountsDialog::AccountsModel::columnCount(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return NUM_ACCOUNTS_MODEL_COLUMNS;
}

QVariant ManageAccountsDialog::AccountsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation == Qt::Vertical) {
        return QVariant(section + 1);
    }

    switch(section)
    {
    case Columns::Type:
        return QStringLiteral("Type");
    case Columns::Username:
        return QStringLiteral("Username");
    case Columns::DisplayName:
        return QStringLiteral("Display name");
    default:
        return QVariant();
    }
}

QVariant ManageAccountsDialog::AccountsModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    int row = index.row();
    int column = index.column();

    int numRows = m_pAvailableAccounts->size();
    if (Q_UNLIKELY((row < 0) || (row >= numRows))) {
        return QVariant();
    }

    const Account & account = (*m_pAvailableAccounts)[row];

    switch(column)
    {
    case Columns::Type:
        {
            if (account.type() == Account::Type::Local) {
                return QStringLiteral("Local");
            }
            else {
                return QStringLiteral("Evernote");
            }
        }
    case Columns::Username:
        return account.name();
    case Columns::DisplayName:
        return account.displayName();
    default:
        return QVariant();
    }
}

bool ManageAccountsDialog::AccountsModel::setData(const QModelIndex & index,
                                                  const QVariant & value, int role)
{
    if (!index.isValid()) {
        return false;
    }

    if (role != Qt::EditRole) {
        return false;
    }

    int row = index.row();
    int column = index.column();

    int numRows = m_pAvailableAccounts->size();
    if (Q_UNLIKELY((row < 0) || (row >= numRows))) {
        return false;
    }

    Account & account = (*m_pAvailableAccounts)[row];

    switch(column)
    {
    case Columns::Type:
        return false;
    case Columns::Username:
        return false;
    case Columns::DisplayName:
        {
            QString displayName = value.toString().trimmed();
            m_stringUtils.removeNewlines(displayName);

            if (account.type() == Account::Type::Evernote)
            {
                int displayNameSize = displayName.size();

                if (displayNameSize < qevercloud::EDAM_USER_NAME_LEN_MIN) {
                    ErrorString error(QT_TRANSLATE_NOOP("", "Account name length is below the acceptable level"));
                    error.details() = QString::number(qevercloud::EDAM_USER_NAME_LEN_MIN);
                    emit badAccountDisplayName(error, row);
                    return false;
                }

                if (displayNameSize > qevercloud::EDAM_USER_NAME_LEN_MAX) {
                    ErrorString error(QT_TRANSLATE_NOOP("", "Account name length is above the acceptable level"));
                    error.details() = QString::number(qevercloud::EDAM_USER_NAME_LEN_MAX);
                    emit badAccountDisplayName(error, row);
                    return false;
                }

                QRegExp regex(qevercloud::EDAM_USER_NAME_REGEX);
                int matchIndex = regex.indexIn(displayName);
                if (matchIndex < 0) {
                    ErrorString error(QT_TRANSLATE_NOOP("", "Account name doesn't match the Evernote's regular expression "
                                                        "for user names; consider simplifying the entered name"));
                    error.details() = QString::number(qevercloud::EDAM_USER_NAME_LEN_MAX);
                    emit badAccountDisplayName(error, row);
                    return false;
                }
            }

            if (account.displayName() == displayName) {
                QNDEBUG(QStringLiteral("The account display name has not really changed"));
                return true;
            }

            account.setDisplayName(displayName);
            emit accountDisplayNameChanged(account);
            return true;
        }
    default:
        return false;
    }
}

ManageAccountsDialog::AccountsModelDelegate::AccountsModelDelegate(QObject * parent) :
    QStyledItemDelegate(parent)
{}

QWidget * ManageAccountsDialog::AccountsModelDelegate::createEditor(QWidget * parent,
                                                                    const QStyleOptionViewItem & option,
                                                                    const QModelIndex & index) const
{
    // Don't allow to edit username and account type
    if (index.isValid() && (index.column() != AccountsModel::Columns::DisplayName)) {
        return Q_NULLPTR;
    }

    return QStyledItemDelegate::createEditor(parent, option, index);
}

#include "ManageAccountsDialog.moc"

} // namespace quentier
