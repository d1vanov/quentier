#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__GENERIC_RESOURCE_DISPLAY_WIDGET_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__GENERIC_RESOURCE_DISPLAY_WIDGET_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QUrl)
QT_FORWARD_DECLARE_CLASS(QFileInfo)

namespace qute_note {

class GenericResourceDisplayWidget: public QWidget
{
    Q_OBJECT
public:
    GenericResourceDisplayWidget(const QString & mimeType, const QUrl & url, const QStringList & argumentNames,
                                 const QStringList & argumentValues,  QWidget * parent = nullptr);

private:
    Q_DISABLE_COPY(GenericResourceDisplayWidget)

private:
    QIcon getIconForFile(const QFileInfo & fileInfo) const;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__GENERIC_RESOURCE_DISPLAY_WIDGET_H
