/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "NoteEditorPluginFactory.h"
#include "GenericResourceDisplayWidget.h"
#include "EncryptedAreaPlugin.h"
#include "NoteEditor_p.h"
#include <quentier/note_editor/ResourceFileStorageManager.h>
#include <quentier/utility/FileIOThreadWorker.h>
#include <quentier/utility/EncryptionManager.h>
#include <quentier/note_editor/DecryptedTextManager.h>
#include <quentier/utility/QuentierCheckPtr.h>
#include <quentier/utility/DesktopServices.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/types/Note.h>
#include <quentier/types/ResourceAdapter.h>
#include <QFileIconProvider>
#include <QDir>
#include <QRegExp>

namespace quentier {

NoteEditorPluginFactory::NoteEditorPluginFactory(NoteEditorPrivate & noteEditor,
                                                 const ResourceFileStorageManager & resourceFileStorageManager,
                                                 const FileIOThreadWorker & fileIOThreadWorker,
                                                 QObject * parent) :
    QWebPluginFactory(parent),
    m_noteEditor(noteEditor),
    m_resourcePlugins(),
    m_lastFreeResourcePluginId(1),
    m_pCurrentNote(Q_NULLPTR),
    m_fallbackResourceIcon(QIcon::fromTheme(QStringLiteral("unknown"))),
    m_mimeDatabase(),
    m_pResourceFileStorageManager(&resourceFileStorageManager),
    m_pFileIOThreadWorker(&fileIOThreadWorker),
    m_resourceIconCache(),
    m_fileSuffixesCache(),
    m_filterStringsCache(),
    m_genericResourceDisplayWidgetPlugins(),
    m_encryptedAreaPlugins()
{}

NoteEditorPluginFactory::~NoteEditorPluginFactory()
{
    QNDEBUG(QStringLiteral("NoteEditorPluginFactory::~NoteEditorPluginFactory"));

    for(auto it = m_genericResourceDisplayWidgetPlugins.begin(), end = m_genericResourceDisplayWidgetPlugins.end(); it != end; ++it)
    {
        QPointer<GenericResourceDisplayWidget> pWidget = *it;
        if (!pWidget.isNull()) {
            pWidget->hide();
            delete pWidget;
        }
    }

    for(auto it = m_encryptedAreaPlugins.begin(), end = m_encryptedAreaPlugins.end(); it != end; ++it)
    {
        QPointer<EncryptedAreaPlugin> pPlugin = *it;
        if (!pPlugin.isNull()) {
            pPlugin->hide();
            delete pPlugin;
        }
    }
}

const NoteEditorPrivate & NoteEditorPluginFactory::noteEditor() const
{
    return m_noteEditor;
}

NoteEditorPluginFactory::ResourcePluginIdentifier NoteEditorPluginFactory::addResourcePlugin(INoteEditorResourcePlugin * plugin,
                                                                                             QNLocalizedString & errorDescription,
                                                                                             const bool forceOverrideTypeKeys)
{
    QNDEBUG(QStringLiteral("NoteEditorPluginFactory::addResourcePlugin: ") << (plugin ? plugin->name() : QStringLiteral("<null>"))
            << QStringLiteral(", force override type keys = ") << (forceOverrideTypeKeys ? QStringLiteral("true") : QStringLiteral("false")));

    if (!plugin) {
        errorDescription = QT_TR_NOOP("detected attempt to install null note editor plugin");
        QNWARNING(errorDescription);
        return 0;
    }

    auto resourcePluginsEnd = m_resourcePlugins.end();
    for(auto it = m_resourcePlugins.begin(); it != resourcePluginsEnd; ++it)
    {
        const INoteEditorResourcePlugin * currentPlugin = it.value();
        if (plugin == currentPlugin) {
            errorDescription = QT_TR_NOOP("detected attempt to install the same resource plugin instance more than once");
            QNWARNING(errorDescription);
            return 0;
        }
    }

    const QStringList mimeTypes = plugin->mimeTypes();
    if (mimeTypes.isEmpty()) {
        errorDescription = QT_TR_NOOP("can't install note editor resource plugin without supported mime types");
        QNWARNING(errorDescription);
        return 0;
    }

    if (!forceOverrideTypeKeys)
    {
        const int numMimeTypes = mimeTypes.size();

        for(auto it = m_resourcePlugins.begin(); it != resourcePluginsEnd; ++it)
        {
            const INoteEditorResourcePlugin * currentPlugin = it.value();
            const QStringList currentPluginMimeTypes = currentPlugin->mimeTypes();

            for(int i = 0; i < numMimeTypes; ++i)
            {
                const QString & mimeType = mimeTypes[i];
                if (currentPluginMimeTypes.contains(mimeType)) {
                    errorDescription = QT_TR_NOOP("can't install note editor resource plugin: found "
                                                  "conflicting mime type from another plugin");
                    errorDescription += QStringLiteral(": ");
                    errorDescription += mimeType;
                    errorDescription += QStringLiteral(" ");
                    errorDescription += QT_TR_NOOP("from plugin");
                    errorDescription += QStringLiteral(" ");
                    errorDescription += currentPlugin->name();
                    QNINFO(errorDescription);
                    return 0;
                }
            }
        }
    }

    plugin->setParent(Q_NULLPTR);

    ResourcePluginIdentifier pluginId = m_lastFreeResourcePluginId;
    ++m_lastFreeResourcePluginId;

    m_resourcePlugins[pluginId] = plugin;

    QNTRACE(QStringLiteral("Assigned id ") << pluginId << QStringLiteral(" to resource plugin ") << plugin->name());
    return pluginId;
}

bool NoteEditorPluginFactory::removeResourcePlugin(const NoteEditorPluginFactory::ResourcePluginIdentifier id,
                                                   QNLocalizedString & errorDescription)
{
    QNDEBUG(QStringLiteral("NoteEditorPluginFactory::removeResourcePlugin: ") << id);

    auto it = m_resourcePlugins.find(id);
    if (it == m_resourcePlugins.end()) {
        errorDescription = QT_TR_NOOP("can't uninstall note editor plugin: plugin with id");
        errorDescription += QStringLiteral(" ");
        errorDescription += QString::number(id);
        errorDescription += QStringLiteral(" ");
        errorDescription += QT_TR_NOOP("was not found");
        QNDEBUG(errorDescription);
        return false;
    }

    INoteEditorResourcePlugin * plugin = it.value();
    QString pluginName = (plugin ? plugin->name() : QStringLiteral("<null>"));
    QNTRACE(QStringLiteral("Plugin to remove: ") << pluginName);

    delete plugin;
    plugin = Q_NULLPTR;

    Q_UNUSED(m_resourcePlugins.erase(it));

    QWebPluginFactory::refreshPlugins();

    QNTRACE(QStringLiteral("Done removing resource plugin ") << id << QStringLiteral(" (") << pluginName << QStringLiteral(")"));
    return true;
}

bool NoteEditorPluginFactory::hasResourcePlugin(const NoteEditorPluginFactory::ResourcePluginIdentifier id) const
{
    auto it = m_resourcePlugins.find(id);
    return (it != m_resourcePlugins.end());
}

bool NoteEditorPluginFactory::hasResourcePluginForMimeType(const QString & mimeType) const
{
    QNDEBUG(QStringLiteral("NoteEditorPluginFactory::hasResourcePluginForMimeType: ") << mimeType);

    if (m_resourcePlugins.empty()) {
        return false;
    }

    auto resourcePluginsEnd = m_resourcePlugins.end();
    for(auto it = m_resourcePlugins.begin(); it != resourcePluginsEnd; ++it)
    {
        const INoteEditorResourcePlugin * plugin = it.value();
        const QStringList & mimeTypes = plugin->mimeTypes();
        if (mimeTypes.contains(mimeType)) {
            return true;
        }
    }

    return false;
}

bool NoteEditorPluginFactory::hasResourcePluginForMimeType(const QRegExp & mimeTypeRegex) const
{
    QNDEBUG(QStringLiteral("NoteEditorPluginFactory::hasResourcePluginForMimeType: ") << mimeTypeRegex.pattern());

    if (m_resourcePlugins.empty()) {
        return false;
    }

    auto resourcePluginsEnd = m_resourcePlugins.end();
    for(auto it = m_resourcePlugins.begin(); it != resourcePluginsEnd; ++it)
    {
        const INoteEditorResourcePlugin * plugin = it.value();
        const QStringList & mimeTypes = plugin->mimeTypes();
        if (!mimeTypes.filter(mimeTypeRegex).isEmpty()) {
            return true;
        }
    }

    return false;
}

void NoteEditorPluginFactory::setNote(const Note & note)
{
    QNDEBUG(QStringLiteral("NoteEditorPluginFactory::setNote: change current note to ") << (note.hasTitle() ? note.title() : note.toString()));
    m_pCurrentNote = &note;
}

void NoteEditorPluginFactory::setFallbackResourceIcon(const QIcon & icon)
{
    m_fallbackResourceIcon = icon;
}

void NoteEditorPluginFactory::setInactive()
{
    QNDEBUG(QStringLiteral("NoteEditorPluginFactory::setInactive"));

    for(auto it = m_genericResourceDisplayWidgetPlugins.begin(), end = m_genericResourceDisplayWidgetPlugins.end(); it != end; ++it)
    {
        QPointer<GenericResourceDisplayWidget> pWidget = *it;
        if (!pWidget.isNull()) {
            pWidget->hide();
        }
    }

    for(auto it = m_encryptedAreaPlugins.begin(), end = m_encryptedAreaPlugins.end(); it != end; ++it)
    {
        QPointer<EncryptedAreaPlugin> pPlugin = *it;
        if (!pPlugin.isNull()) {
            pPlugin->hide();
        }
    }
}

void NoteEditorPluginFactory::setActive()
{
    QNDEBUG(QStringLiteral("NoteEditorPluginFactory::setActive"));

    for(auto it = m_genericResourceDisplayWidgetPlugins.begin(), end = m_genericResourceDisplayWidgetPlugins.end(); it != end; ++it)
    {
        QPointer<GenericResourceDisplayWidget> pWidget = *it;
        if (!pWidget.isNull()) {
            pWidget->show();
        }
    }

    for(auto it = m_encryptedAreaPlugins.begin(), end = m_encryptedAreaPlugins.end(); it != end; ++it)
    {
        QPointer<EncryptedAreaPlugin> pPlugin = *it;
        if (!pPlugin.isNull()) {
            pPlugin->show();
        }
    }
}

void NoteEditorPluginFactory::updateResource(const IResource & resource)
{
    QNDEBUG(QStringLiteral("NoteEditorPluginFactory::updateResource: ") << resource);

    auto it = std::find_if(m_genericResourceDisplayWidgetPlugins.begin(),
                           m_genericResourceDisplayWidgetPlugins.end(),
                           GenericResourceDisplayWidgetFinder(resource));
    if (it != m_genericResourceDisplayWidgetPlugins.end())
    {
        QPointer<GenericResourceDisplayWidget> pWidget = *it;
        if (Q_UNLIKELY(pWidget.isNull())) {
            return;
        }

        pWidget->updateResourceName(resource.displayName());
    }
}

QObject * NoteEditorPluginFactory::create(const QString & pluginType, const QUrl & url,
                                          const QStringList & argumentNames,
                                          const QStringList & argumentValues) const
{
    QNDEBUG(QStringLiteral("NoteEditorPluginFactory::create: pluginType = ") << pluginType
            << QStringLiteral(", url = ") << url.toString() << QStringLiteral(", argument names: ")
            << argumentNames.join(QStringLiteral(", ")) << QStringLiteral(", argument values: ")
            << argumentValues.join(QStringLiteral(", ")));

    if (!m_pCurrentNote) {
        QNFATAL(QStringLiteral("Can't create note editor plugin: no note specified"));
        return Q_NULLPTR;
    }

    if (pluginType == RESOURCE_PLUGIN_HTML_OBJECT_TYPE) {
        return createResourcePlugin(argumentNames, argumentValues);
    }
    else if (pluginType == ENCRYPTED_AREA_PLUGIN_OBJECT_TYPE) {
        return createEncryptedAreaPlugin(argumentNames, argumentValues);
    }

    QNWARNING(QStringLiteral("Can't create note editor plugin: plugin type is not identified: ") << pluginType);
    return Q_NULLPTR;
}

QObject * NoteEditorPluginFactory::createResourcePlugin(const QStringList & argumentNames,
                                                        const QStringList & argumentValues) const
{
    QNDEBUG(QStringLiteral("NoteEditorPluginFactory::createResourcePlugin: argument names = ") << argumentNames.join(QStringLiteral(","))
            << QStringLiteral("; argument values = ") << argumentValues.join(QStringLiteral(",")));

    int resourceHashIndex = argumentNames.indexOf(QStringLiteral("hash"));
    if (resourceHashIndex < 0) {
        QNFATAL(QStringLiteral("Can't create note editor resource plugin: hash argument was not found"));
        return Q_NULLPTR;
    }

    int resourceMimeTypeIndex = argumentNames.indexOf(QStringLiteral("resource-mime-type"));
    if (resourceMimeTypeIndex < 0) {
        QNFATAL(QStringLiteral("Can't create note editor resource plugin: resource-mime-type argument not found"));
        return Q_NULLPTR;
    }

    QByteArray resourceHash = QByteArray::fromHex(argumentValues.at(resourceHashIndex).toLocal8Bit());
    QString resourceMimeType = argumentValues.at(resourceMimeTypeIndex);

    QList<ResourceAdapter> resourceAdapters = m_pCurrentNote->resourceAdapters();

    const ResourceAdapter * pCurrentResource = Q_NULLPTR;

    const int numResources = resourceAdapters.size();
    for(int i = 0; i < numResources; ++i)
    {
        const ResourceAdapter & resource = resourceAdapters[i];
        if (!resource.hasDataHash()) {
            continue;
        }

        if (resource.dataHash() == resourceHash) {
            pCurrentResource = &resource;
            break;
        }
    }

    if (!pCurrentResource) {
        QNFATAL(QStringLiteral("Can't find resource in note by data hash: ") << resourceHash.toHex()
                << QStringLiteral(", note: ") << *m_pCurrentNote);
        return Q_NULLPTR;
    }

    QNTRACE(QStringLiteral("Number of installed resource plugins: ") << m_resourcePlugins.size());

    if (!m_resourcePlugins.isEmpty())
    {
        // Need to loop through installed resource plugins considering the last installed plugins first
        // Sadly, Qt doesn't support proper reverse iterators for its own containers without STL compatibility so will emulate them
        auto resourcePluginsBegin = m_resourcePlugins.begin();
        auto resourcePluginsBeforeBegin = resourcePluginsBegin;
        --resourcePluginsBeforeBegin;

        auto resourcePluginsLast = m_resourcePlugins.end();
        --resourcePluginsLast;

        for(auto it = resourcePluginsLast; it != resourcePluginsBeforeBegin; --it)
        {
            const INoteEditorResourcePlugin * plugin = it.value();

            const QStringList mimeTypes = plugin->mimeTypes();
            QNTRACE(QStringLiteral("Testing resource plugin ") << plugin->name() << QStringLiteral(" with id ") << it.key()
                    << QStringLiteral(", mime types: ") << mimeTypes.join(QStringLiteral("; ")));

            if (mimeTypes.contains(resourceMimeType))
            {
                QNTRACE(QStringLiteral("Will use plugin ") << plugin->name());
                INoteEditorResourcePlugin * newPlugin = plugin->clone();
                QNLocalizedString errorDescription;
                bool res = newPlugin->initialize(resourceMimeType, argumentNames, argumentValues, *this,
                                                 *pCurrentResource, errorDescription);
                if (!res) {
                    QNINFO(QStringLiteral("Can't initialize note editor resource plugin ") << plugin->name()
                           << QStringLiteral(": ") << errorDescription);
                    delete newPlugin;
                    continue;
                }

                return newPlugin;
            }
        }
    }

    QNTRACE(QStringLiteral("Haven't found any installed resource plugin supporting mime type ") << resourceMimeType
            << QStringLiteral(", will use generic resource display plugin for that"));

    QString resourceDisplayName;
    if (pCurrentResource->hasResourceAttributes())
    {
        const qevercloud::ResourceAttributes & attributes = pCurrentResource->resourceAttributes();
        if (attributes.fileName.isSet()) {
            resourceDisplayName = attributes.fileName;
        }
        else if (attributes.sourceURL.isSet()) {
            resourceDisplayName = attributes.sourceURL;
        }
    }

    QString resourceDataSize;
    if (pCurrentResource->hasDataBody()) {
        const QByteArray & data = pCurrentResource->dataBody();
        quint64 bytes = static_cast<quint64>(data.size());
        resourceDataSize = humanReadableSize(bytes);
    }
    else if (pCurrentResource->hasAlternateDataBody()) {
        const QByteArray & data = pCurrentResource->alternateDataBody();
        quint64 bytes = static_cast<quint64>(data.size());
        resourceDataSize = humanReadableSize(bytes);
    }

    auto cachedIconIt = m_resourceIconCache.find(resourceMimeType);
    if (cachedIconIt == m_resourceIconCache.end()) {
        QIcon resourceIcon = getIconForMimeType(resourceMimeType);
        cachedIconIt = m_resourceIconCache.insert(resourceMimeType, resourceIcon);
    }

    QStringList fileSuffixes;
    auto fileSuffixesIt = m_fileSuffixesCache.find(resourceMimeType);
    if (fileSuffixesIt == m_fileSuffixesCache.end()) {
        fileSuffixes = getFileSuffixesForMimeType(resourceMimeType);
        m_fileSuffixesCache[resourceMimeType] = fileSuffixes;
    }

    QString filterString;
    auto filterStringIt = m_filterStringsCache.find(resourceMimeType);
    if (filterStringIt == m_filterStringsCache.end()) {
        filterString = getFilterStringForMimeType(resourceMimeType);
        m_filterStringsCache[resourceMimeType] = filterString;
    }

    QWidget * pParentWidget = qobject_cast<QWidget*>(parent());
    GenericResourceDisplayWidget * pGenericResourceDisplayWidget = new GenericResourceDisplayWidget(pParentWidget);
    QObject::connect(pGenericResourceDisplayWidget, QNSIGNAL(GenericResourceDisplayWidget,openResourceRequest,const QByteArray&),
                     &m_noteEditor, QNSLOT(NoteEditorPrivate,openAttachment,const QByteArray&));

    // NOTE: upon return this generic resource display widget would be reparented to the caller anyway,
    // the parent setting above is strictly for possible use within initialize method (for example, if
    // the widget would need to create some dialog window, it could be modal due to the existence of the parent)

    pGenericResourceDisplayWidget->initialize(cachedIconIt.value(), resourceDisplayName,
                                              resourceDataSize, fileSuffixes, filterString,
                                              *pCurrentResource, *m_pResourceFileStorageManager,
                                              *m_pFileIOThreadWorker);

    m_genericResourceDisplayWidgetPlugins.push_back(QPointer<GenericResourceDisplayWidget>(pGenericResourceDisplayWidget));
    return pGenericResourceDisplayWidget;
}

QObject * NoteEditorPluginFactory::createEncryptedAreaPlugin(const QStringList & argumentNames, const QStringList & argumentValues) const
{
    QWidget * pParentWidget = qobject_cast<QWidget*>(parent());
    EncryptedAreaPlugin * pEncryptedAreaPlugin = new EncryptedAreaPlugin(m_noteEditor, pParentWidget);

    QNLocalizedString errorDescription;
    bool res = pEncryptedAreaPlugin->initialize(argumentNames, argumentValues, *this, errorDescription);
    if (!res) {
        QNINFO(QStringLiteral("Can't initialize note editor encrypted area plugin ") << pEncryptedAreaPlugin->name()
               << QStringLiteral(": ") << errorDescription);
        delete pEncryptedAreaPlugin;
        return Q_NULLPTR;
    }

    m_encryptedAreaPlugins.push_back(QPointer<EncryptedAreaPlugin>(pEncryptedAreaPlugin));
    return pEncryptedAreaPlugin;
}

QList<QWebPluginFactory::Plugin> NoteEditorPluginFactory::plugins() const
{
    QList<QWebPluginFactory::Plugin> plugins;

    QWebPluginFactory::Plugin resourceDisplayPlugin;
    resourceDisplayPlugin.name = QStringLiteral("Resource display plugin");
    QWebPluginFactory::MimeType resourceObjectType;
    resourceObjectType.name = RESOURCE_PLUGIN_HTML_OBJECT_TYPE;
    resourceDisplayPlugin.mimeTypes = QList<QWebPluginFactory::MimeType>() << resourceObjectType;
    plugins.push_back(resourceDisplayPlugin);

    QWebPluginFactory::Plugin encryptedAreaPlugin;
    encryptedAreaPlugin.name = QStringLiteral("Encrypted area plugin");
    QWebPluginFactory::MimeType encryptedAreaObjectType;
    encryptedAreaObjectType.name = ENCRYPTED_AREA_PLUGIN_OBJECT_TYPE;
    encryptedAreaPlugin.mimeTypes = QList<QWebPluginFactory::MimeType>() << encryptedAreaObjectType;
    plugins.push_back(encryptedAreaPlugin);

    return plugins;
}

QIcon NoteEditorPluginFactory::getIconForMimeType(const QString & mimeTypeName) const
{
    QNDEBUG(QStringLiteral("NoteEditorPluginFactory::getIconForMimeType: mime type name = ") << mimeTypeName);

    QMimeType mimeType = m_mimeDatabase.mimeTypeForName(mimeTypeName);
    if (!mimeType.isValid()) {
        QNTRACE(QStringLiteral("Couldn't find valid mime type object for name/alias ") << mimeTypeName
                << QStringLiteral(", will use \"unknown\" icon"));
        return m_fallbackResourceIcon;
    }

    QString iconName = mimeType.iconName();
    if (QIcon::hasThemeIcon(iconName)) {
        QNTRACE(QStringLiteral("Found icon from theme, name = ") << iconName);
        return QIcon::fromTheme(iconName, m_fallbackResourceIcon);
    }

    iconName = mimeType.genericIconName();
    if (QIcon::hasThemeIcon(iconName)) {
        QNTRACE(QStringLiteral("Found generic icon from theme, name = ") << iconName);
        return QIcon::fromTheme(iconName, m_fallbackResourceIcon);
    }

    QStringList suffixes;
    auto fileSuffixesIt = m_fileSuffixesCache.find(mimeTypeName);
    if (fileSuffixesIt == m_fileSuffixesCache.end()) {
        suffixes = getFileSuffixesForMimeType(mimeTypeName);
        m_fileSuffixesCache[mimeTypeName] = suffixes;
    }
    else {
        suffixes = fileSuffixesIt.value();
    }

    const int numSuffixes = suffixes.size();
    if (numSuffixes == 0) {
        QNDEBUG(QStringLiteral("Can't find any file suffix for mime type ") << mimeTypeName
                << QStringLiteral(", will use \"unknown\" icon"));
        return m_fallbackResourceIcon;
    }

    bool hasNonEmptySuffix = false;
    for(int i = 0; i < numSuffixes; ++i)
    {
        const QString & suffix = suffixes[i];
        if (suffix.isEmpty()) {
            QNTRACE(QStringLiteral("Found empty file suffix within suffixes, skipping it"));
            continue;
        }

        hasNonEmptySuffix = true;
        break;
    }

    if (!hasNonEmptySuffix) {
        QNDEBUG(QStringLiteral("All file suffixes for mime type ") << mimeTypeName << QStringLiteral(" are empty, will use \"unknown\" icon"));
        return m_fallbackResourceIcon;
    }

    // The implementation uses the "fake files" with mime-type specific suffixes in order to try to get icons
    // corresponding to the bespoke mime types from QFileIconProvider

    QString fakeFilesStoragePath = applicationPersistentStoragePath();
    fakeFilesStoragePath.append(QStringLiteral("/fake_files"));

    QDir fakeFilesDir(fakeFilesStoragePath);
    if (!fakeFilesDir.exists())
    {
        QNDEBUG(QStringLiteral("Fake files storage path doesn't exist yet, will attempt to create it"));
        if (!fakeFilesDir.mkpath(fakeFilesStoragePath)) {
            QNWARNING(QStringLiteral("Can't create fake files storage path folder"));
            return m_fallbackResourceIcon;
        }
    }

    QString filename(QStringLiteral("fake_file"));
    QFileInfo fileInfo;
    for(int i = 0; i < numSuffixes; ++i)
    {
        const QString & suffix = suffixes[i];
        if (suffix.isEmpty()) {
            continue;
        }

        fileInfo.setFile(fakeFilesDir, filename + QStringLiteral(".") + suffix);
        if (fileInfo.exists() && !fileInfo.isFile())
        {
            if (!fakeFilesDir.rmpath(fakeFilesStoragePath + QStringLiteral("/") + filename + QStringLiteral(".") + suffix)) {
                QNWARNING(QStringLiteral("Can't remove directory ") << fileInfo.absolutePath()
                          << QStringLiteral(" which should not be here in the first place..."));
                continue;
            }
        }

        if (!fileInfo.exists())
        {
            QFile fakeFile(fakeFilesStoragePath + QStringLiteral("/") + filename + QStringLiteral(".") + suffix);
            if (!fakeFile.open(QIODevice::ReadWrite)) {
                QNWARNING(QStringLiteral("Can't open file ") << fakeFilesStoragePath << QStringLiteral("/") << filename
                          << QStringLiteral(".") << suffix << QStringLiteral(" for writing "));
                continue;
            }
        }

        QFileIconProvider fileIconProvider;
        QIcon icon = fileIconProvider.icon(fileInfo);
        if (icon.isNull()) {
            QNTRACE(QStringLiteral("File icon provider returned null icon for file with suffix ") << suffix);
        }

        QNTRACE(QStringLiteral("Returning the icon from file icon provider for mime type ") << mimeTypeName);
        return icon;
    }

    QNTRACE(QStringLiteral("Couldn't find appropriate icon from either icon theme or fake file with QFileIconProvider, "
                           "using \"unknown\" icon as a last resort"));
    return m_fallbackResourceIcon;
}

QStringList NoteEditorPluginFactory::getFileSuffixesForMimeType(const QString & mimeTypeName) const
{
    QNDEBUG(QStringLiteral("NoteEditorPluginFactory::getFileSuffixesForMimeType: mime type name = ") << mimeTypeName);

    QMimeType mimeType = m_mimeDatabase.mimeTypeForName(mimeTypeName);
    if (!mimeType.isValid()) {
        QNTRACE(QStringLiteral("Couldn't find valid mime type object for name/alias ") << mimeTypeName);
        return QStringList();
    }

    return mimeType.suffixes();
}

QString NoteEditorPluginFactory::getFilterStringForMimeType(const QString & mimeTypeName) const
{
    QNDEBUG(QStringLiteral("NoteEditorPluginFactory::getFilterStringForMimeType: mime type name = ") << mimeTypeName);

    QMimeType mimeType = m_mimeDatabase.mimeTypeForName(mimeTypeName);
    if (!mimeType.isValid()) {
        QNTRACE(QStringLiteral("Couldn't find valid mime type object for name/alias ") << mimeTypeName);
        return QString();
    }

    return mimeType.filterString();
}

NoteEditorPluginFactory::GenericResourceDisplayWidgetFinder::GenericResourceDisplayWidgetFinder(const IResource & resource) :
    m_resourceLocalUid(resource.localUid())
{}

bool NoteEditorPluginFactory::GenericResourceDisplayWidgetFinder::operator()(const QPointer<GenericResourceDisplayWidget> & ptr) const
{
    if (ptr.isNull()) {
        return false;
    }

    return (ptr->resourceLocalUid() == m_resourceLocalUid);
}

} // namespace quentier
