#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__NOTE_EDITOR_PLUGIN_FACTORY_P_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__NOTE_EDITOR_PLUGIN_FACTORY_P_H

#include <qute_note/note_editor/NoteEditorPluginFactory.h>
#include <QMimeDatabase>
#include <QIcon>
#include <QThread>
#include <QHash>

QT_FORWARD_DECLARE_CLASS(QRegExp)

namespace qute_note {

class NoteEditorPluginFactoryPrivate: public QObject
{
    Q_OBJECT
public:
    explicit NoteEditorPluginFactoryPrivate(NoteEditorPluginFactory & factory,
                                            const NoteEditor & noteEditor,
                                            const ResourceFileStorageManager & ResourceFileStorageManager,
                                            const FileIOThreadWorker & fileIOThreadWorker,
                                            INoteEditorEncryptedAreaPlugin * pEncryptedAreaPlugin,
                                            QObject * parent = Q_NULLPTR);
    virtual ~NoteEditorPluginFactoryPrivate();

    const NoteEditor & noteEditor() const;

    typedef NoteEditorPluginFactory::ResourcePluginIdentifier ResourcePluginIdentifier;

    ResourcePluginIdentifier addResourcePlugin(INoteEditorResourcePlugin * plugin, QString & errorDescription,
                                               const bool forceOverrideTypeKeys);

    bool removeResourcePlugin(const ResourcePluginIdentifier id, QString & errorDescription);

    bool hasResourcePlugin(const ResourcePluginIdentifier id) const;
    bool hasResourcePluginForMimeType(const QString & mimeType) const;
    bool hasResourcePluginForMimeType(const QRegExp & mimeTypeRegex) const;

    void setNote(const Note & note);

    void setFallbackResourceIcon(const QIcon & icon);

    void setGenericResourceDisplayWidget(IGenericResourceDisplayWidget * genericResourceDisplayWidget);

    void setEncryptedAreaPlugin(INoteEditorEncryptedAreaPlugin * encryptedAreaPlugin);

    QObject * create(const QString & pluginType, const QUrl & url,
                     const QStringList & argumentNames,
                     const QStringList & argumentValues) const;

    QList<QWebPluginFactory::Plugin> plugins() const;

private:
    QObject * createResourcePlugin(const QStringList & argumentNames, const QStringList & argumentValues) const;
    QObject * createEncryptedAreaPlugin(const QStringList & argumentNames, const QStringList & argumentValues) const;

    QIcon getIconForMimeType(const QString & mimeTypeName) const;
    QStringList getFileSuffixesForMimeType(const QString & mimeType) const;
    QString getFilterStringForMimeType(const QString & mimeType) const;

private:
    const NoteEditor &                              m_noteEditor;
    IGenericResourceDisplayWidget *                 m_genericResourceDisplayWidget;

    typedef QHash<ResourcePluginIdentifier, INoteEditorResourcePlugin*> ResourcePluginsHash;
    ResourcePluginsHash                             m_resourcePlugins;
    ResourcePluginIdentifier                        m_lastFreeResourcePluginId;

    const Note *                                    m_pCurrentNote;

    QIcon                                           m_fallbackResourceIcon;

    QMimeDatabase                                   m_mimeDatabase;

    const ResourceFileStorageManager *              m_pResourceFileStorageManager;
    const FileIOThreadWorker *                      m_pFileIOThreadWorker;

    INoteEditorEncryptedAreaPlugin *                m_pEncryptedAreaPlugin;

    mutable QHash<QString, QIcon>                   m_resourceIconCache;
    mutable QHash<QString, QStringList>             m_fileSuffixesCache;
    mutable QHash<QString, QString>                 m_filterStringsCache;

    NoteEditorPluginFactory * const                 q_ptr;
    Q_DECLARE_PUBLIC(NoteEditorPluginFactory)
};

} // namespace qute_note;

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__NOTE_EDITOR_PLUGIN_FACTORY_P_H
