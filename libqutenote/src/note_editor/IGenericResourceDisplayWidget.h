#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__I_GENERIC_RESOURCE_DISPLAY_WIDGET_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__I_GENERIC_RESOURCE_DISPLAY_WIDGET_H

#include <qute_note/utility/Qt4Helper.h>
#include <QWidget>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(IResource)
QT_FORWARD_DECLARE_CLASS(ResourceFileStorageManager)
QT_FORWARD_DECLARE_CLASS(FileIOThreadWorker)

/**
 * The IGenericResourceDisplayWidget class is the interface for any class wishing
 * to implement the generic resource display widget i.e. widget displaying the resources
 * for which there are no resource-specific widgets implementing INoteEditorPlugin interface
 */
class IGenericResourceDisplayWidget: public QWidget
{
    Q_OBJECT
public:
    IGenericResourceDisplayWidget(QWidget * parent = Q_NULLPTR);
    virtual ~IGenericResourceDisplayWidget();

    virtual void initialize(const QIcon & icon, const QString & name,
                            const QString & size, const QStringList & preferredFileSuffixes,
                            const QString & filterString, const IResource & resource,
                            const ResourceFileStorageManager & resourceFileStorageManager,
                            const FileIOThreadWorker & fileIOThreadWorker) = 0;

    virtual IGenericResourceDisplayWidget * create() const = 0;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__I_GENERIC_RESOURCE_DISPLAY_WIDGET_H
