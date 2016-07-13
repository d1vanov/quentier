#ifndef LIB_QUENTIER_NOTE_EDITOR_I_NOTE_EDITOR_RESOURCE_PLUGIN_H
#define LIB_QUENTIER_NOTE_EDITOR_I_NOTE_EDITOR_RESOURCE_PLUGIN_H

#include <quentier/utility/Qt4Helper.h>
#include <quentier/utility/QNLocalizedString.h>
#include <QWidget>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(IResource)
QT_FORWARD_DECLARE_CLASS(NoteEditorPluginFactory)

/**
 * @brief The INoteEditorResourcePlugin class is the interface for note editor plugin
 * implementing the widget displaying the resources of certain mime types
 * built in the note. For example, such plugin could represent the embedded pdf viewer,
 * embedded video viewer etc.
 */
class INoteEditorResourcePlugin: public QWidget
{
protected:
    explicit INoteEditorResourcePlugin(QWidget * parent = Q_NULLPTR);

public:
    /**
     * @brief clone - pure virtual method cloning the current plugin
     * @return pointer to the new clone of the plugin. NOTE: it is
     * caller's responsibility to take care about the ownership
     * of the returned pointer
     */
    virtual INoteEditorResourcePlugin * clone() const = 0;

    /**
     * @brief initialize - the method used to initialize the note editor plugin
     * @param mimeType - mime type of the resource data meant to be displayed by the plugin
     * @param parameterNames - names of string parameters stored in HTML <object> tag for the plugin
     * @param parameterValues - values of string parameters stored in HTML <object> tag for the plugin
     * @param pluginFactory - plugin factory object which initializes plugins; here intended to be used
     * for setting up the signal-slot connections, if necessary
     * @param resource - const reference to the resource which needs to be displayed by the plugin
     * @param errorDescription - error description in case the plugin can't be initialized properly
     * with this set of parameters
     * @return true if initialization was successful, false otherwise
     */
    virtual bool initialize(const QString & mimeType,
                            const QStringList & parameterNames,
                            const QStringList & parameterValues,
                            const NoteEditorPluginFactory & pluginFactory,
                            const IResource & resource,
                            QNLocalizedString & errorDescription) = 0;

    /**
     * @brief mimeTypes - the method telling which are the mime types of the resources the plugin can work with
     * @return the list of resource mime types the plugin supports
     */
    virtual QStringList mimeTypes() const = 0;

    /**
     * @brief fileExtensions - the method telling which file extensions of the data
     * the plugin should be able to handle mapped to mime types the plugin supports as well
     * @return the hash of file extensions the plugin supports per mime types the plugin supports
     */
    virtual QHash<QString, QStringList> fileExtensions() const = 0;

    /**
     * @brief name - the method returning the name of the plugin
     * @return the name of the plugin
     */
    virtual QString name() const = 0;

    /**
     * @brief description - the optional method
     * @return plugin's description
     */
    virtual QString description() const { return QString(); }
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_I_NOTE_EDITOR_RESOURCE_PLUGIN_H
