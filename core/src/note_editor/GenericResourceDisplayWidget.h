#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__GENERIC_RESOURCE_DISPLAY_WIDGET_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__GENERIC_RESOURCE_DISPLAY_WIDGET_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QMimeDatabase)

namespace Ui {
class GenericResourceDisplayWidget;
}

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(IResource)

class GenericResourceDisplayWidget: public QWidget
{
    Q_OBJECT
public:
    GenericResourceDisplayWidget(const QIcon & icon, const QString & name,
                                 const QString & size, const QStringList & preferredFileSuffixes,
                                 const QString & mimeTypeName,
                                 const IResource & resource,
                                 QWidget * parent = nullptr);

private:
    Q_DISABLE_COPY(GenericResourceDisplayWidget)

private Q_SLOTS:
    void onOpenWithButtonPressed();
    void onSaveAsButtonPressed();

private:
    Ui::GenericResourceDisplayWidget *  m_pUI;

    const IResource *       m_resource;
    const QStringList       m_preferredFileSuffixes;
    const QString           m_mimeTypeName;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__GENERIC_RESOURCE_DISPLAY_WIDGET_H
