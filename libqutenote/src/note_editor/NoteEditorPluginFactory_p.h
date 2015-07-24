#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_PLUGIN_FACTORY_PRIVATE_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_PLUGIN_FACTORY_PRIVATE_H

#include "NoteEditorPluginFactory.h"
#include <QMimeDatabase>
#include <QIcon>
#include <QThread>

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
                                            QObject * parent = nullptr);
    virtual ~NoteEditorPluginFactoryPrivate();

    const NoteEditor & noteEditor() const;

    typedef NoteEditorPluginFactory::PluginIdentifier PluginIdentifier;

    PluginIdentifier addPlugin(INoteEditorPlugin * plugin, QString & errorDescription,
                               const bool forceOverrideTypeKeys);

    bool removePlugin(const PluginIdentifier id, QString & errorDescription);

    bool hasPlugin(const PluginIdentifier id) const;
    bool hasPluginForMimeType(const QString & mimeType) const;
    bool hasPluginForMimeType(const QRegExp & mimeTypeRegex) const;

    void setNote(const Note & note);

    void setFallbackResourceIcon(const QIcon & icon);

    QObject * create(const QString & mimeType, const QUrl & url,
                     const QStringList & argumentNames,
                     const QStringList & argumentValues) const;

    QList<QWebPluginFactory::Plugin> plugins() const;

private:
    QIcon getIconForMimeType(const QString & mimeTypeName) const;
    QStringList getFileSuffixesForMimeType(const QString & mimeType) const;
    QString getFilterStringForMimeType(const QString & mimeType) const;

private:
    const NoteEditor &                              m_noteEditor;
    QHash<PluginIdentifier, INoteEditorPlugin*>     m_plugins;
    PluginIdentifier                                m_lastFreePluginId;
    const Note *                                    m_pCurrentNote;
    QIcon                                           m_fallbackResourceIcon;
    QMimeDatabase                                   m_mimeDatabase;

    QHash<QString, INoteEditorPlugin*>              m_specificParameterPlugins;

    const ResourceFileStorageManager *              m_pResourceFileStorageManager;
    const FileIOThreadWorker *                      m_pFileIOThreadWorker;

    mutable QHash<QString, QIcon>                   m_resourceIconCache;
    mutable QHash<QString, QStringList>             m_fileSuffixesCache;
    mutable QHash<QString, QString>                 m_filterStringsCache;

    NoteEditorPluginFactory * const                 q_ptr;
    Q_DECLARE_PUBLIC(NoteEditorPluginFactory)

private:
    // Memory cache helpers
    mutable QString         m_parameterKeyCache;
};

} // namespace qute_note;

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_PLUGIN_FACTORY_PRIVATE_H
