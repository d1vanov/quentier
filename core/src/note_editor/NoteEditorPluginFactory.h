#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_PLUGIN_FACTORY_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_PLUGIN_FACTORY_H

#include "INoteEditorPlugin.h"
#include <QWebPluginFactory>

QT_FORWARD_DECLARE_CLASS(QRegExp)

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(NoteEditorPluginFactoryPrivate)

/**
 * @brief The NoteEditorPluginFactory class allows one to install and uninstall custom plugins to/from NoteEditor
 */
class QUTE_NOTE_EXPORT NoteEditorPluginFactory: public QWebPluginFactory
{
    Q_OBJECT
public:
    explicit NoteEditorPluginFactory(QObject * parent = nullptr);
    virtual ~NoteEditorPluginFactory();

    /**
     * @brief PluginIdentifier - the unique identifier of the plugin assigned to it by the factory;
     * any identifier larger than zero is a "valid" one while zero is the same as "no identifier"
     */
    typedef quint32 PluginIdentifier;

    /**
     * @brief addPlugin - the method allowing one to add new custom plugins to NoteEditor
     * @param plugin - plugin to be added to NoteEditor, subclass of INoteEditorPlugin.
     * Note: the ownership of the pointer object is transferred to the factory
     * @param errorDescription - error description in case the plugin can't be installed
     * @param forceOverrideTypeKeys - when multiple plugins support multiple myme types and file extensions,
     * they can potentially overlap. In such a case if this parameters if set to false (which is its default value),
     * the method would refuse to install the plugin conflicting with other plugins by mime types and/or
     * file extensions. However, if this parameter is set to true, the newly installed plugin would override
     * the mime types and/or file extensions of previously installed plugins. It can be rolled back
     * when the plugin is uninstalled.
     * @return non-zero plugin identifier if the plugin was successfully installed, zero otherwise
     */
    PluginIdentifier addPlugin(INoteEditorPlugin * plugin, QString & errorDescription,
                               const bool forceOverrideTypeKeys = false);

    /**
     * @brief removePlugin - the method used to uninstall previously installed plugin to NoteEditor
     * @param id - identifier of the plugin to be uninstalled
     * @param errorDescription - error description if the plugin couldn't be uninstalled
     * @return true if the plugin was successfully uninstalled, false otherwise
     */
    bool removePlugin(const PluginIdentifier id, QString & errorDescription);

    /**
     * @brief hasPlugin - the method allowing one to find out whether the plugin with certain identifier
     * is currently installed or now
     * @param id - identifier of the plugin to be checked for being installed
     * @return true if the plugin is installed, false otherwise
     */
    bool hasPlugin(const PluginIdentifier id) const;

    /**
     * @brief hasPluginForMimeType - the method allowing one to find out whether the resource display plugin
     * corresponding to certain mime type is currently installed
     * @param mimeType - mime type for which the presence of the install plugin is checked
     * @return true if the plugin for specified mime type is installed, false otherwise
     */
    bool hasPluginForMimeType(const QString & mimeType) const;

    /**
     * @brief hasPluginForMimeType - the method allowing one to find out whether the resource display plugin
     * corresponding to any mime type matching the specified regex is currently installed
     * @param mimeTypeRegex - the regex for mime type the matching to which for any installed plugin is checked
     * @return true if the plugin for mime type matching the specified regex is installed, false otherwise
     */
    bool hasPluginForMimeType(const QRegExp & mimeTypeRegex) const;

    /**
     * @brief setNote - note editor plugin factory needs to access certain information from the resources
     * of current note for which the plugins are created. For that the factory needs to have a const reference
     * to current note. This method provides such a reference.
     * @param note - current note to be displayed by note editor with plugins from note editor plugin factory
     */
    void setNote(const Note & note);

    /**
     * @brief setFallbackResourceIcon - note editor plugin factory would create the "generic"
     * resource display plugin if it finds no "real" plugin installed for given mime type.
     * This "generic" plugin would try to figure out the best matching icon for given mime type
     * but it can fail to do so. In such a case the icon specified by this method would be displayed
     * for the resource with such unidentified mime type
     * @param icon - icon to be used as a last resort for resources of unidentified mime types
     */
    void setFallbackResourceIcon(const QIcon & icon);

private:
    // QWebPluginFactory interface
    virtual QObject * create(const QString & mimeType, const QUrl & url,
                             const QStringList & argumentNames,
                             const QStringList & argumentValues) const;

    virtual QList<QWebPluginFactory::Plugin> plugins() const;

private:
    NoteEditorPluginFactoryPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(NoteEditorPluginFactory)
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_PLUGIN_FACTORY_H
