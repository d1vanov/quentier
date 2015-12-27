#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__NOTE_EDITOR_PLUGIN_FACTORY_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__NOTE_EDITOR_PLUGIN_FACTORY_H

#include "INoteEditorResourcePlugin.h"
#include <QWebPluginFactory>
#include <QMimeDatabase>
#include <QIcon>
#include <QHash>
#include <QSharedPointer>
#include <QPointer>

QT_FORWARD_DECLARE_CLASS(QRegExp)

#define RESOURCE_PLUGIN_HTML_OBJECT_TYPE "application/vnd.qutenote.resource"
#define ENCRYPTED_AREA_PLUGIN_OBJECT_TYPE "application/vnd.qutenote.encrypt"

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(ResourceFileStorageManager)
QT_FORWARD_DECLARE_CLASS(FileIOThreadWorker)
QT_FORWARD_DECLARE_CLASS(EncryptionManager)
QT_FORWARD_DECLARE_CLASS(DecryptedTextManager)
QT_FORWARD_DECLARE_CLASS(NoteEditorPrivate)

QT_FORWARD_DECLARE_CLASS(EncryptedAreaPlugin)
QT_FORWARD_DECLARE_CLASS(GenericResourceDisplayWidget)

/**
 * @brief The NoteEditorPluginFactory class allows one to install and uninstall custom plugins to/from NoteEditor
 */
class NoteEditorPluginFactory: public QWebPluginFactory
{
    Q_OBJECT
public:
    explicit NoteEditorPluginFactory(NoteEditorPrivate & editor,
                                     const ResourceFileStorageManager & resourceFileStorageManager,
                                     const FileIOThreadWorker & fileIOThreadWorker,
                                     QObject * parent = Q_NULLPTR);
    virtual ~NoteEditorPluginFactory();

    /**
     * @brief noteEditor - the accessor providing the const reference to INoteEditorBackend object owning the factory
     * @return const reference to INoteEditorBackend object
     */
    const NoteEditorPrivate & noteEditor() const;

    /**
     * @brief ResourcePluginIdentifier - the unique identifier of the plugin assigned to it by the factory;
     * any identifier larger than zero is a "valid" one while zero is the same as "no identifier"
     */
    typedef quint32 ResourcePluginIdentifier;

    /**
     * @brief addResourcePlugin - the method allowing one to add new custom plugins to NoteEditor
     * @param plugin - plugin to be added to NoteEditor, subclass of INoteEditorResourcePlugin.
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
    ResourcePluginIdentifier addResourcePlugin(INoteEditorResourcePlugin * plugin, QString & errorDescription,
                                               const bool forceOverrideTypeKeys = false);

    /**
     * @brief removeResourcePlugin - the method used to uninstall previously installed plugin to NoteEditor
     * @param id - identifier of the plugin to be uninstalled
     * @param errorDescription - error description if the plugin couldn't be uninstalled
     * @return true if the plugin was successfully uninstalled, false otherwise
     */
    bool removeResourcePlugin(const ResourcePluginIdentifier id, QString & errorDescription);

    /**
     * @brief hasResourcePlugin - the method allowing one to find out whether the plugin with certain identifier
     * is currently installed or now
     * @param id - identifier of the plugin to be checked for being installed
     * @return true if the plugin is installed, false otherwise
     */
    bool hasResourcePlugin(const ResourcePluginIdentifier id) const;

    /**
     * @brief hasResourcePluginForMimeType - the method allowing one to find out whether the resource display plugin
     * corresponding to certain mime type is currently installed
     * @param mimeType - mime type for which the presence of the install plugin is checked
     * @return true if the plugin for specified mime type is installed, false otherwise
     */
    bool hasResourcePluginForMimeType(const QString & mimeType) const;

    /**
     * @brief hasResourcePluginForMimeType - the method allowing one to find out whether the resource display plugin
     * corresponding to any mime type matching the specified regex is currently installed
     * @param mimeTypeRegex - the regex for mime type the matching to which for any installed plugin is checked
     * @return true if the plugin for mime type matching the specified regex is installed, false otherwise
     */
    bool hasResourcePluginForMimeType(const QRegExp & mimeTypeRegex) const;

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

    void setInactive();
    void setActive();

    void updateResource(const IResource & resource);

private:
    // QWebPluginFactory interface
    virtual QObject * create(const QString & mimeType, const QUrl & url,
                             const QStringList & argumentNames,
                             const QStringList & argumentValues) const;

    virtual QList<QWebPluginFactory::Plugin> plugins() const;

private:
    QObject * createResourcePlugin(const QStringList & argumentNames, const QStringList & argumentValues) const;
    QObject * createEncryptedAreaPlugin(const QStringList & argumentNames, const QStringList & argumentValues) const;

    QIcon getIconForMimeType(const QString & mimeTypeName) const;
    QStringList getFileSuffixesForMimeType(const QString & mimeType) const;
    QString getFilterStringForMimeType(const QString & mimeType) const;

private:
    class GenericResourceDisplayWidgetFinder
    {
    public:
        GenericResourceDisplayWidgetFinder(const IResource & resource);
        bool operator()(const QPointer<GenericResourceDisplayWidget> & ptr) const;

    private:
        QString m_resourceLocalGuid;
    };

private:
    NoteEditorPrivate &                                 m_noteEditor;

    typedef QHash<ResourcePluginIdentifier, INoteEditorResourcePlugin*> ResourcePluginsHash;
    ResourcePluginsHash                                 m_resourcePlugins;
    ResourcePluginIdentifier                            m_lastFreeResourcePluginId;

    const Note *                                        m_pCurrentNote;

    QIcon                                               m_fallbackResourceIcon;

    QMimeDatabase                                       m_mimeDatabase;

    const ResourceFileStorageManager *                  m_pResourceFileStorageManager;
    const FileIOThreadWorker *                          m_pFileIOThreadWorker;

    mutable QHash<QString, QIcon>                       m_resourceIconCache;
    mutable QHash<QString, QStringList>                 m_fileSuffixesCache;
    mutable QHash<QString, QString>                     m_filterStringsCache;

    mutable QVector<QPointer<GenericResourceDisplayWidget> >      m_genericResourceDisplayWidgetPlugins;
    mutable QVector<QPointer<EncryptedAreaPlugin> >               m_encryptedAreaPlugins;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__NOTE_EDITOR_PLUGIN_FACTORY_H
