#ifndef QUENTIER_WIDGETS_TAG_FILTER_WIDGET_H
#define QUENTIER_WIDGETS_TAG_FILTER_WIDGET_H

#include <quentier/types/Account.h>
#include <QWidget>
#include <QPointer>

QT_FORWARD_DECLARE_CLASS(QModelIndex)
QT_FORWARD_DECLARE_CLASS(FlowLayout)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(TagModel)

class TagsFilterWidget: public QWidget
{
    Q_OBJECT
public:
    explicit TagsFilterWidget(QWidget * parent = Q_NULLPTR);

    const Account & account() const { return m_account; }
    void setAccount(const Account & account);

    /**
     * @brief setTagModel - method setting the tag model to the widget;
     * NOTE: it is intended to set the account before setting the model
     */
    void setTagModel(TagModel * pTagModel);

    QStringList tagNames() const;

Q_SIGNALS:
    void addedTag(const QString & tagName);
    void tagRemoved(const QString & tagName);

private Q_SLOTS:
    // Slots to track the model changes
    void onModelDataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight
#if QT_VERSION < 0x050000
                            );
#else
                            , const QVector<int> & roles = QVector<int>());
#endif

    void onModelRowsRemoved(const QModelIndex & parent, int first, int last);
    void onModelReset();

private:
    FlowLayout *            m_pLayout;
    Account                 m_account;
    QPointer<TagModel>      m_pTagModel;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_TAG_FILTER_WIDGET_H
