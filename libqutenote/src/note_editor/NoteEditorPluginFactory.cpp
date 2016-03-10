#include "NoteEditorPluginFactory.h"
#include "GenericResourceDisplayWidget.h"
#include "EncryptedAreaPlugin.h"
#include "NoteEditor_p.h"
#include <qute_note/note_editor/ResourceFileStorageManager.h>
#include <qute_note/utility/FileIOThreadWorker.h>
#include <qute_note/utility/EncryptionManager.h>
#include <qute_note/note_editor/DecryptedTextManager.h>
#include <qute_note/utility/QuteNoteCheckPtr.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <qute_note/types/Note.h>
#include <qute_note/types/ResourceAdapter.h>
#include <QFileIconProvider>
#include <QDir>
#include <QRegExp>

namespace qute_note {

NoteEditorPluginFactory::NoteEditorPluginFactory(NoteEditorPrivate & noteEditor,
                                                 const ResourceFileStorageManager & resourceFileStorageManager,
                                                 const FileIOThreadWorker & fileIOThreadWorker,
                                                 QObject * parent) :
    QWebPluginFactory(parent),
    m_noteEditor(noteEditor),
    m_resourcePlugins(),
    m_lastFreeResourcePluginId(1),
    m_pCurrentNote(Q_NULLPTR),
    m_fallbackResourceIcon(QIcon::fromTheme("unknown")),
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
    QNDEBUG("NoteEditorPluginFactory::~NoteEditorPluginFactory");

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
                                                                                             QString & errorDescription,
                                                                                             const bool forceOverrideTypeKeys)
{
    QNDEBUG("NoteEditorPluginFactory::addResourcePlugin: " << (plugin ? plugin->name() : QString("<null>"))
            << ", force override type keys = " << (forceOverrideTypeKeys ? "true" : "false"));

    if (!plugin) {
        errorDescription = QT_TR_NOOP("Detected attempt to install null note editor plugin");
        QNWARNING(errorDescription);
        return 0;
    }

    auto resourcePluginsEnd = m_resourcePlugins.end();
    for(auto it = m_resourcePlugins.begin(); it != resourcePluginsEnd; ++it)
    {
        const INoteEditorResourcePlugin * currentPlugin = it.value();
        if (plugin == currentPlugin) {
            errorDescription = QT_TR_NOOP("Detected attempt to install the same resource plugin instance more than once");
            QNWARNING(errorDescription);
            return 0;
        }
    }

    const QStringList mimeTypes = plugin->mimeTypes();
    if (mimeTypes.isEmpty()) {
        errorDescription = QT_TR_NOOP("Can't install note editor resource plugin without supported mime types");
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
                    errorDescription = QT_TR_NOOP("Can't install note editor resource plugin: found "
                                                  "conflicting mime type " + mimeType +
                                                  " from plugin " + currentPlugin->name());
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

    QNTRACE("Assigned id " << pluginId << " to resource plugin " << plugin->name());
    return pluginId;
}

bool NoteEditorPluginFactory::removeResourcePlugin(const NoteEditorPluginFactory::ResourcePluginIdentifier id,
                                                   QString & errorDescription)
{
    QNDEBUG("NoteEditorPluginFactory::removeResourcePlugin: " << id);

    auto it = m_resourcePlugins.find(id);
    if (it == m_resourcePlugins.end()) {
        errorDescription = QT_TR_NOOP("Can't uninstall note editor plugin: plugin with id " +
                                      QString::number(id) + " was not found");
        QNDEBUG(errorDescription);
        return false;
    }

    INoteEditorResourcePlugin * plugin = it.value();
    QString pluginName = (plugin ? plugin->name() : QString("<null>"));
    QNTRACE("Plugin to remove: " << pluginName);

    delete plugin;
    plugin = Q_NULLPTR;

    Q_UNUSED(m_resourcePlugins.erase(it));

    QWebPluginFactory::refreshPlugins();

    QNTRACE("Done removing resource plugin " << id << " (" << pluginName << ")");
    return true;
}

bool NoteEditorPluginFactory::hasResourcePlugin(const NoteEditorPluginFactory::ResourcePluginIdentifier id) const
{
    auto it = m_resourcePlugins.find(id);
    return (it != m_resourcePlugins.end());
}

bool NoteEditorPluginFactory::hasResourcePluginForMimeType(const QString & mimeType) const
{
    QNDEBUG("NoteEditorPluginFactory::hasResourcePluginForMimeType: " << mimeType);

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
    QNDEBUG("NoteEditorPluginFactory::hasResourcePluginForMimeType: " << mimeTypeRegex.pattern());

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
    QNDEBUG("NoteEditorPluginFactory::setNote: change current note to " << (note.hasTitle() ? note.title() : note.ToQString()));
    m_pCurrentNote = &note;
}

void NoteEditorPluginFactory::setFallbackResourceIcon(const QIcon & icon)
{
    m_fallbackResourceIcon = icon;
}

void NoteEditorPluginFactory::setInactive()
{
    QNDEBUG("NoteEditorPluginFactory::setInactive");

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
    QNDEBUG("NoteEditorPluginFactory::setActive");

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
    QNDEBUG("NoteEditorPluginFactory::updateResource: " << resource);

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
    QNDEBUG("NoteEditorPluginFactory::create: pluginType = " << pluginType
            << ", url = " << url.toString() << ", argument names: " << argumentNames.join(", ")
            << ", argument values: " << argumentValues.join(", "));

    if (!m_pCurrentNote) {
        QNFATAL("Can't create note editor plugin: no note specified");
        return Q_NULLPTR;
    }

    if (pluginType == RESOURCE_PLUGIN_HTML_OBJECT_TYPE) {
        return createResourcePlugin(argumentNames, argumentValues);
    }
    else if (pluginType == ENCRYPTED_AREA_PLUGIN_OBJECT_TYPE) {
        return createEncryptedAreaPlugin(argumentNames, argumentValues);
    }

    QNWARNING("Can't create note editor plugin: plugin type is not identified: " << pluginType);
    return Q_NULLPTR;
}

QObject * NoteEditorPluginFactory::createResourcePlugin(const QStringList & argumentNames,
                                                        const QStringList & argumentValues) const
{
    QNDEBUG("NoteEditorPluginFactory::createResourcePlugin: argument names = " << argumentNames.join(",")
            << "; argument values = " << argumentValues.join(","));

    int resourceHashIndex = argumentNames.indexOf("hash");
    if (resourceHashIndex < 0) {
        QNFATAL("Can't create note editor resource plugin: hash argument was not found");
        return Q_NULLPTR;
    }

    int resourceMimeTypeIndex = argumentNames.indexOf("resource-mime-type");
    if (resourceMimeTypeIndex < 0) {
        QNFATAL("Can't create note editor resource plugin: resource-mime-type argument not found");
        return Q_NULLPTR;
    }

    QString resourceHash = argumentValues.at(resourceHashIndex);
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
        QNFATAL("Can't find resource in note by data hash: " << resourceHash
                << ", note: " << *m_pCurrentNote);
        return Q_NULLPTR;
    }

    QNTRACE("Number of installed resource plugins: " << m_resourcePlugins.size());

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
            QNTRACE("Testing resource plugin " << plugin->name() << " with id " << it.key()
                    << ", mime types: " << mimeTypes.join("; "));

            if (mimeTypes.contains(resourceMimeType))
            {
                QNTRACE("Will use plugin " << plugin->name());
                INoteEditorResourcePlugin * newPlugin = plugin->clone();
                QString errorDescription;
                bool res = newPlugin->initialize(resourceMimeType, argumentNames, argumentValues, *this,
                                                 *pCurrentResource, errorDescription);
                if (!res) {
                    QNINFO("Can't initialize note editor resource plugin " << plugin->name() << ": " << errorDescription);
                    delete newPlugin;
                    continue;
                }

                return newPlugin;
            }
        }
    }

    QNTRACE("Haven't found any installed resource plugin supporting mime type " << resourceMimeType
            << ", will use generic resource display plugin for that");

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
        int bytes = data.size();
        resourceDataSize = humanReadableSize(bytes);
    }
    else if (pCurrentResource->hasAlternateDataBody()) {
        const QByteArray & data = pCurrentResource->alternateDataBody();
        int bytes = data.size();
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

    QString errorDescription;
    bool res = pEncryptedAreaPlugin->initialize(argumentNames, argumentValues, *this, errorDescription);
    if (!res) {
        QNINFO("Can't initialize note editor encrypted area plugin " << pEncryptedAreaPlugin->name() << ": " << errorDescription);
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
    resourceDisplayPlugin.name = "Resource display plugin";
    QWebPluginFactory::MimeType resourceObjectType;
    resourceObjectType.name = RESOURCE_PLUGIN_HTML_OBJECT_TYPE;
    resourceDisplayPlugin.mimeTypes = QList<QWebPluginFactory::MimeType>() << resourceObjectType;
    plugins.push_back(resourceDisplayPlugin);

    QWebPluginFactory::Plugin encryptedAreaPlugin;
    encryptedAreaPlugin.name = "Encrypted area plugin";
    QWebPluginFactory::MimeType encryptedAreaObjectType;
    encryptedAreaObjectType.name = ENCRYPTED_AREA_PLUGIN_OBJECT_TYPE;
    encryptedAreaPlugin.mimeTypes = QList<QWebPluginFactory::MimeType>() << encryptedAreaObjectType;
    plugins.push_back(encryptedAreaPlugin);

    return plugins;
}

QIcon NoteEditorPluginFactory::getIconForMimeType(const QString & mimeTypeName) const
{
    QNDEBUG("NoteEditorPluginFactory::getIconForMimeType: mime type name = " << mimeTypeName);

    QMimeType mimeType = m_mimeDatabase.mimeTypeForName(mimeTypeName);
    if (!mimeType.isValid()) {
        QNTRACE("Couldn't find valid mime type object for name/alias " << mimeTypeName
                << ", will use \"unknown\" icon");
        return m_fallbackResourceIcon;
    }

    QString iconName = mimeType.iconName();
    if (QIcon::hasThemeIcon(iconName)) {
        QNTRACE("Found icon from theme, name = " << iconName);
        return QIcon::fromTheme(iconName, m_fallbackResourceIcon);
    }

    iconName = mimeType.genericIconName();
    if (QIcon::hasThemeIcon(iconName)) {
        QNTRACE("Found generic icon from theme, name = " << iconName);
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
        QNDEBUG("Can't find any file suffix for mime type " << mimeTypeName
                << ", will use \"unknown\" icon");
        return m_fallbackResourceIcon;
    }

    bool hasNonEmptySuffix = false;
    for(int i = 0; i < numSuffixes; ++i)
    {
        const QString & suffix = suffixes[i];
        if (suffix.isEmpty()) {
            QNTRACE("Found empty file suffix within suffixes, skipping it");
            continue;
        }

        hasNonEmptySuffix = true;
        break;
    }

    if (!hasNonEmptySuffix) {
        QNDEBUG("All file suffixes for mime type " << mimeTypeName << " are empty, will use \"unknown\" icon");
        return m_fallbackResourceIcon;
    }

    QString fakeFilesStoragePath = applicationPersistentStoragePath();
    fakeFilesStoragePath.append("/fake_files");

    QDir fakeFilesDir(fakeFilesStoragePath);
    if (!fakeFilesDir.exists())
    {
        QNDEBUG("Fake files storage path doesn't exist yet, will attempt to create it");
        if (!fakeFilesDir.mkpath(fakeFilesStoragePath)) {
            QNWARNING("Can't create fake files storage path folder");
            return m_fallbackResourceIcon;
        }
    }

    QString filename("fake_file");
    QFileInfo fileInfo;
    for(int i = 0; i < numSuffixes; ++i)
    {
        const QString & suffix = suffixes[i];
        if (suffix.isEmpty()) {
            continue;
        }

        fileInfo.setFile(fakeFilesDir, filename + "." + suffix);
        if (fileInfo.exists() && !fileInfo.isFile())
        {
            if (!fakeFilesDir.rmpath(fakeFilesStoragePath + "/" + filename + "." + suffix)) {
                QNWARNING("Can't remove directory " << fileInfo.absolutePath()
                          << " which should not be here in the first place...");
                continue;
            }
        }

        if (!fileInfo.exists())
        {
            QFile fakeFile(fakeFilesStoragePath + "/" + filename + "." + suffix);
            if (!fakeFile.open(QIODevice::ReadWrite)) {
                QNWARNING("Can't open file " << fakeFilesStoragePath << "/" << filename
                          << "." << suffix << " for writing ");
                continue;
            }
        }

        QFileIconProvider fileIconProvider;
        QIcon icon = fileIconProvider.icon(fileInfo);
        if (icon.isNull()) {
            QNTRACE("File icon provider returned null icon for file with suffix " << suffix);
        }

        QNTRACE("Returning the icon from file icon provider for mime type " << mimeTypeName);
        return icon;
    }

    QNTRACE("Couldn't find appropriate icon from either icon theme or fake file with QFileIconProvider, "
            "using \"unknown\" icon as a last resort");
    return m_fallbackResourceIcon;
}

QStringList NoteEditorPluginFactory::getFileSuffixesForMimeType(const QString & mimeTypeName) const
{
    QNDEBUG("NoteEditorPluginFactory::getFileSuffixesForMimeType: mime type name = " << mimeTypeName);

    QMimeType mimeType = m_mimeDatabase.mimeTypeForName(mimeTypeName);
    if (!mimeType.isValid()) {
        QNTRACE("Couldn't find valid mime type object for name/alias " << mimeTypeName);
        return QStringList();
    }

    return mimeType.suffixes();
}

QString NoteEditorPluginFactory::getFilterStringForMimeType(const QString & mimeTypeName) const
{
    QNDEBUG("NoteEditorPluginFactory::getFilterStringForMimeType: mime type name = " << mimeTypeName);

    QMimeType mimeType = m_mimeDatabase.mimeTypeForName(mimeTypeName);
    if (!mimeType.isValid()) {
        QNTRACE("Couldn't find valid mime type object for name/alias " << mimeTypeName);
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

} // namespace qute_note
