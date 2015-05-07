#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_PLUGIN_FACTORY_PRIVATE_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_PLUGIN_FACTORY_PRIVATE_H

#include "NoteEditorPluginFactory.h"
#include <QMimeDatabase>
#include <QIcon>
#include <QThread>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(FileIOThreadWorker)

class NoteEditorPluginFactoryPrivate: public QObject
{
    Q_OBJECT
public:
    explicit NoteEditorPluginFactoryPrivate(NoteEditorPluginFactory & factory,
                                            QObject * parent = nullptr);
    virtual ~NoteEditorPluginFactoryPrivate();

    typedef NoteEditorPluginFactory::PluginIdentifier PluginIdentifier;

    PluginIdentifier addPlugin(INoteEditorPlugin * plugin, QString & errorDescription,
                               const bool forceOverrideTypeKeys);

    bool removePlugin(const PluginIdentifier id, QString & errorDescription);

    bool hasPlugin(const PluginIdentifier id) const;

    void setNote(const Note & note);

    void setFallbackResourceIcon(const QIcon & icon);

    QObject * create(const QString & mimeType, const QUrl & url,
                     const QStringList & argumentNames,
                     const QStringList & argumentValues) const;

    QList<QWebPluginFactory::Plugin> plugins() const;

private:
    QIcon getIconForMimeType(const QString & mimeTypeName) const;
    QStringList getFileSuffixesForMimeType(const QString & mimeType) const;

private:
    QHash<PluginIdentifier, INoteEditorPlugin*>     m_plugins;
    PluginIdentifier                                m_lastFreePluginId;
    const Note *                                    m_pCurrentNote;
    QIcon                                           m_fallbackResourceIcon;
    QMimeDatabase                                   m_mimeDatabase;

    QThread *                                       m_pIOThread;
    FileIOThreadWorker *                            m_pFileIOThreadWorker;

    mutable QHash<QString, QIcon>                   m_resourceIconCache;
    mutable QHash<QString, QStringList>             m_fileSuffixesCache;

    NoteEditorPluginFactory * const                 q_ptr;
    Q_DECLARE_PUBLIC(NoteEditorPluginFactory)
};

} // namespace qute_note;

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_PLUGIN_FACTORY_PRIVATE_H
