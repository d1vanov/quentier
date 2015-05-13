#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_PLUGIN_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_PLUGIN_H

#include <tools/Linkage.h>
#include <QWidget>
#include <QUrl>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(IResource)

/**
 * @brief The INoteEditorPlugin class is the interface for any class wishing to implement
 * the display for any custom object embedded inside the note editor,
 * such as embedded pdf viewer, embedded video viewer etc.
 */
class QUTE_NOTE_EXPORT INoteEditorPlugin: public QWidget
{
protected:
    explicit INoteEditorPlugin(QWidget * parent = nullptr);

public:
    /**
     * @brief clone - pure virtual method cloning the current plugin
     * @return pointer to the new clone of the plugin. NOTE: there's no need to worry about the ownership
     * over the cloned plugin, it is caller's responsibility to take care about it
     */
    virtual INoteEditorPlugin * clone() const = 0;

    /**
     * @brief initialize - the method used to initialize the note editor plugin
     * @param mimeType - mime type of the data meant to be displayed by the plugin
     * @param url - url of the content to be displayed by the plugin
     * @param parameterNames - names of string parameters stored in HTML <object> tag for the plugin
     * @param parameterValues - values of string parameters stored in HTML <object> tag for the plugin
     * @param resource - const reference to resource which actually needs to be displayed
     * @param errorDescription - error description in case the plugin can't be initialized properly
     * with this set of parameters
     * @return true if initialization was successful, false otherwise
     */
    virtual bool initialize(const QString & mimeType, const QUrl & url,
                            const QStringList & parameterNames,
                            const QStringList & parameterValues,
                            const IResource * resource,
                            QString & errorDescription) = 0;

    /**
     * @brief mimeTypes - the method telling which mime types the plugin can work with
     * @return the list of mime types the plugin supports
     */
    virtual QStringList mimeTypes() const = 0;

    /**
     * @brief fileExtensions - the method telling which file extensions of the data
     * the plugin should be able to handle mapped to mime types the plugin supports as well
     * @return the hash of file extensions the plugin supports per mime types the plugin supports
     */
    virtual QHash<QString, QStringList> fileExtensions() const = 0;

    /**
     * @brief specificAttributes - the method should be used by plugins which do not
     * really represent resources hence the concept of mime type doesn't make much sense for them;
     * @return the list of specific HTML attributes which presence within the note editor's content
     * should trigger the insertion of the plugin; the default implementation in INoteEditorPlugin
     * returns the empty list of strings. NOTE: the plugin using specific HTML attributes
     * should return the empty list of mime types in mimeTypes() method and the empty list of file extensions
     * in fileExtensions() method, otherwise the specific HTML attributes of the plugin would be ignored
     */
    virtual QStringList specificAttributes() const;

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

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_PLUGIN_H
