#include "NoteEditor_p.h"
#include "NoteEditorPage.h"
#include "NoteEditorPluginFactory.h"
#include <client/types/Note.h>
#include <client/types/Notebook.h>
#include <client/enml/ENMLConverter.h>
#include <logging/QuteNoteLogger.h>
#include <QWebFrame>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QByteArray>
#include <QImage>
#include <QDropEvent>
#include <QMimeType>
#include <QMimeData>
#include <QMimeDatabase>

namespace qute_note {

NoteEditorPrivate::NoteEditorPrivate(NoteEditor & noteEditor) :
    QObject(&noteEditor),
    m_jQuery(),
    m_resizableColumnsPlugin(),
    m_onFixedWidthTableResize(),
    m_getSelectionHtml(),
    m_replaceSelectionWithHtml(),
    m_pNote(nullptr),
    m_pNotebook(nullptr),
    m_modified(false),
    m_encryptionManager(new EncryptionManager),
    m_decryptedTextCache(new QHash<QString, QPair<QString, bool> >()),
    m_enmlConverter(),
    m_pluginFactory(nullptr),
    m_pagePrefix("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">"
                 "<html><head>"
                 "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">"
                 "<title></title></head>"),
    m_htmlCachedMemory(),
    m_errorCachedMemory(),
    q_ptr(&noteEditor)
{
    Q_Q(NoteEditor);

    NoteEditorPage * page = new NoteEditorPage(*q);
    page->settings()->setAttribute(QWebSettings::LocalContentCanAccessFileUrls, true);
    page->settings()->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls, true);
    page->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
    page->setContentEditable(true);

    m_pluginFactory = new NoteEditorPluginFactory(page);
    page->setPluginFactory(m_pluginFactory);

    q->setPage(page);

    // Setting initial "blank" page, it is of great importance in order to make image insertion work
    q->setHtml(m_pagePrefix + "<body></body></html>");

    q->setAcceptDrops(true);

    __initNoteEditorResources();

    QFile file(":/javascript/jquery/jquery-2.1.3.min.js");
    file.open(QIODevice::ReadOnly);
    m_jQuery = file.readAll();
    file.close();

    file.setFileName(":/javascript/colResizable/colResizable-1.5.min.js");
    file.open(QIODevice::ReadOnly);
    m_resizableColumnsPlugin = file.readAll();
    file.close();

    file.setFileName(":/javascript/scripts/onFixedWidthTableResize.js");
    file.open(QIODevice::ReadOnly);
    m_onFixedWidthTableResize = file.readAll();
    file.close();

    file.setFileName(":/javascript/scripts/getSelectionHtml.js");
    file.open(QIODevice::ReadOnly);
    m_getSelectionHtml = file.readAll();
    file.close();

    file.setFileName(":/javascript/scripts/replaceSelectionWithHtml.js");
    file.open(QIODevice::ReadOnly);
    m_replaceSelectionWithHtml = file.readAll();
    file.close();

    QObject::connect(page, SIGNAL(contentsChanged()), q, SIGNAL(contentChanged()));
    QObject::connect(q, SIGNAL(loadFinished(bool)), this, SLOT(onNoteLoadFinished(bool)));
    QObject::connect(this, SIGNAL(notifyError(QString)), q, SIGNAL(notifyError(QString)));

    // TODO: temporary thing for debugging/development, remove later
    onNoteLoadFinished(true);
}

NoteEditorPrivate::~NoteEditorPrivate()
{
    delete m_pNote;
    delete m_pNotebook;
}

void NoteEditorPrivate::onNoteLoadFinished(bool ok)
{
    if (!ok) {
        QNWARNING("Note page was not loaded successfully");
        return;
    }

    Q_Q(NoteEditor);
    QWebFrame * frame = q->page()->mainFrame();
    if (!frame) {
        return;
    }

    Q_UNUSED(frame->evaluateJavaScript(m_jQuery));
    Q_UNUSED(frame->evaluateJavaScript(m_resizableColumnsPlugin));
    Q_UNUSED(frame->evaluateJavaScript(m_onFixedWidthTableResize));
    Q_UNUSED(frame->evaluateJavaScript(m_getSelectionHtml));
    Q_UNUSED(frame->evaluateJavaScript(m_replaceSelectionWithHtml));
    QNTRACE("Evaluated all JavaScript helper functions");
}

void NoteEditorPrivate::clearContent()
{
    QNDEBUG("NoteEditorPrivate::clearContent");

    Q_Q(NoteEditor);
    q->setHtml(m_pagePrefix + "<body></body></html>");
}

void NoteEditorPrivate::updateColResizableTableBindings()
{
    QNDEBUG("NoteEditorPrivate::updateColResizableTableBindings");

    Q_Q(NoteEditor);
    bool readOnly = !q->page()->isContentEditable();

    QString colResizable = "$(\"table\").colResizable({";
    if (readOnly) {
        colResizable += "disable:true});";
    }
    else {
        colResizable += "liveDrag:true, "
                        "gripInnerHtml:\"<div class=\\'grip\\'></div>\", "
                        "draggingClass:\"dragging\", "
                        "postbackSafe:true, "
                        "onResize:onFixedWidthTableResized"
                        "});";
    }

    QNTRACE("colResizable js code: " << colResizable);
    q->page()->mainFrame()->evaluateJavaScript(colResizable);
}

void NoteEditorPrivate::htmlToNoteContent()
{
    QNDEBUG("NoteEditorPrivate::htmlToNoteContent");

    if (!m_pNote) {
        QNTRACE("No note is set to note editor");
        return;
    }

    if (m_pNote->hasActive() && !m_pNote->active()) {
        m_errorCachedMemory = QT_TR_NOOP("Current note is marked as read-only, the changes won't be saved");
        QNWARNING(m_errorCachedMemory << ", note: local guid = " << m_pNote->localGuid()
                  << ", guid = " << (m_pNote->hasGuid() ? m_pNote->guid() : "<null>")
                  << ", title = " << (m_pNote->hasTitle() ? m_pNote->title() : "<null>"));
        emit notifyError(m_errorCachedMemory);
        return;
    }

    if (m_pNotebook && m_pNotebook->hasRestrictions())
    {
        const qevercloud::NotebookRestrictions & restrictions = m_pNotebook->restrictions();
        if (restrictions.noUpdateNotes.isSet() && restrictions.noUpdateNotes.ref()) {
            m_errorCachedMemory = QT_TR_NOOP("The notebook the current note belongs to doesn't allow notes modification, "
                                             "the changes won't be saved");
            QNWARNING(m_errorCachedMemory << ", note: local guid = " << m_pNote->localGuid()
                      << ", guid = " << (m_pNote->hasGuid() ? m_pNote->guid() : "<null>")
                      << ", title = " << (m_pNote->hasTitle() ? m_pNote->title() : "<null>")
                      << ", notebook: local guid = " << m_pNotebook->localGuid() << ", guid = "
                      << (m_pNotebook->hasGuid() ? m_pNotebook->guid() : "<null>") << ", name = "
                      << (m_pNotebook->hasName() ? m_pNotebook->name() : "<null>"));
            emit notifyError(m_errorCachedMemory);
            return;
        }
    }

    Q_Q(const NoteEditor);
    m_htmlCachedMemory = q->page()->mainFrame()->toHtml();
    bool res = m_enmlConverter.htmlToNoteContent(m_htmlCachedMemory, *m_pNote, m_errorCachedMemory);
    if (!res) {
        QNWARNING("Can't convert note editor page's content to HTML: " << m_errorCachedMemory);
        emit notifyError(m_errorCachedMemory);
        return;
    }
}

QVariant NoteEditorPrivate::execJavascriptCommandWithResult(const QString & command)
{
    Q_Q(NoteEditor);
    QWebFrame * frame = q->page()->mainFrame();
    QString javascript = QString("document.execCommand(\"%1\", false, null)").arg(command);
    QVariant result = frame->evaluateJavaScript(javascript);
    QNTRACE("Executed javascript command: " << javascript << ", result = " << result.toString());
    return result;
}

void NoteEditorPrivate::execJavascriptCommand(const QString & command)
{
    Q_UNUSED(execJavascriptCommandWithResult(command));
}

QVariant NoteEditorPrivate::execJavascriptCommandWithResult(const QString & command, const QString & args)
{
    Q_Q(NoteEditor);
    QWebFrame * frame = q->page()->mainFrame();
    QString javascript = QString("document.execCommand('%1', false, '%2')").arg(command).arg(args);
    QVariant result = frame->evaluateJavaScript(javascript);
    QNTRACE("Executed javascript command: " << javascript << ", result = " << result.toString());
    return result;
}

void NoteEditorPrivate::execJavascriptCommand(const QString & command, const QString & args)
{
    Q_UNUSED(execJavascriptCommandWithResult(command, args));
}

void NoteEditorPrivate::setNoteAndNotebook(const Note & note, const Notebook & notebook)
{
    QNDEBUG("NoteEditorPrivate::setNoteAndNotebook: note: local guid = " << note.localGuid()
            << ", guid = " << (note.hasGuid() ? note.guid() : "<null>") << ", title: "
            << (note.hasTitle() ? note.title() : "<null>") << "; notebook: local guid = "
            << notebook.localGuid() << ", guid = " << (notebook.hasGuid() ? notebook.guid() : "<null>")
            << ", name = " << (notebook.hasName() ? notebook.name() : "<null>"));

    // FIXME: if the editor wasn't empty, need to clean it carefully, ensure there'd be no dangling JavaScript functions for some gone elements

    if (!m_pNote) {
        m_pNote = new Note(note);
    }
    else {
        *m_pNote = note;
    }

    if (!m_pNotebook) {
        m_pNotebook = new Notebook(notebook);
    }
    else {
        *m_pNotebook = notebook;
    }

    if (!m_pNote->hasContent()) {
        QNDEBUG("Note without content was inserted into NoteEditor");
        clearContent();
        return;
    }

    m_htmlCachedMemory.resize(0);
    bool res = m_enmlConverter.noteContentToHtml(*m_pNote, m_htmlCachedMemory, m_errorCachedMemory,
                                                 m_decryptedTextCache);
    if (!res)
    {
        QNWARNING("Can't convert note's content to HTML: " << m_errorCachedMemory);
        emit notifyError(m_errorCachedMemory);
        clearContent();
        return;
    }

    int bodyTagIndex = m_htmlCachedMemory.indexOf("<body>");
    if (bodyTagIndex < 0) {
        m_errorCachedMemory = QT_TR_NOOP("can't find <body> tag in the result of note to HTML conversion");
        QNWARNING(m_errorCachedMemory << ", note content: " << m_pNote->content()
                  << ", html: " << m_htmlCachedMemory);
        emit notifyError(m_errorCachedMemory);
        clearContent();
        return;
    }

    m_htmlCachedMemory.remove(0, bodyTagIndex);
    m_htmlCachedMemory.prepend(m_pagePrefix);
    int bodyClosingTagIndex = m_htmlCachedMemory.indexOf("</body>");
    if (bodyClosingTagIndex < 0) {
        m_errorCachedMemory = QT_TR_NOOP("Can't find </body> tag in the result of note to HTML conversion");
        QNWARNING(m_errorCachedMemory << ", note content: " << m_pNote->content()
                  << ", html: " << m_htmlCachedMemory);
        emit notifyError(m_errorCachedMemory);
        clearContent();
        return;
    }

    m_htmlCachedMemory.remove(bodyClosingTagIndex, 7);
    m_htmlCachedMemory.insert(bodyClosingTagIndex, "</html>");

    m_htmlCachedMemory.replace("<br></br>", "</br>");   // Webkit-specific fix

    Q_Q(NoteEditor);

    bool readOnly = false;
    if (m_pNote->hasActive() && !m_pNote->active()) {
        QNDEBUG("Current note is not active, setting it to read-only state");
        q->page()->setContentEditable(false);
        readOnly = true;
    }
    else if (m_pNotebook->hasRestrictions())
    {
        const qevercloud::NotebookRestrictions & restrictions = m_pNotebook->restrictions();
        if (restrictions.noUpdateNotes.isSet() && restrictions.noUpdateNotes.ref())
        {
            QNDEBUG("Notebook restrictions forbid the note modification, setting note's content to read-only state");
            q->page()->setContentEditable(false);
            readOnly = true;
        }
    }

    if (!readOnly) {
        QNDEBUG("Nothing prevents user to modify the note, allowing it in the editor");
        q->page()->setContentEditable(true);
    }

    q->setHtml(m_htmlCachedMemory);
    updateColResizableTableBindings();
    QNTRACE("Done setting the current note and notebook");
}

const Note * NoteEditorPrivate::getNote() const
{
    if (!m_pNote) {
        return nullptr;
    }

    if (m_modified) {
        // TODO: convert widget's content to Note's ENML and resources
    }

    return m_pNote;
}

const Notebook * NoteEditorPrivate::getNotebook() const
{
    return m_pNotebook;
}

bool NoteEditorPrivate::isModified() const
{
    return m_modified;
}

const NoteEditorPluginFactory & NoteEditorPrivate::pluginFactory() const
{
    return *m_pluginFactory;
}

NoteEditorPluginFactory & NoteEditorPrivate::pluginFactory()
{
    return *m_pluginFactory;
}

void NoteEditorPrivate::onDropEvent(QDropEvent *pEvent)
{
    QNDEBUG("NoteEditorPrivate::onDropEvent");

    if (!pEvent) {
        QNINFO("Null drop event was detected");
        return;
    }

    const QMimeData * pMimeData = pEvent->mimeData();
    if (!pMimeData) {
        QNINFO("Null mime data from drop event was detected");
        return;
    }

    QList<QUrl> urls = pMimeData->urls();
    typedef QList<QUrl>::iterator Iter;
    Iter urlsEnd = urls.end();
    for(Iter it = urls.begin(); it != urlsEnd; ++it)
    {
        QString url = it->toString();
        if (url.toLower().startsWith("file://")) {
            url.remove(0,6);
            dropFile(url);
        }
    }
}

template <typename T>
QString NoteEditorPrivate::composeHtmlTable(const T width, const T singleColumnWidth,
                                            const int rows, const int columns,
                                            const bool relative)
{
    // Table header
    QString htmlTable = "<div><table style=\"border-collapse: collapse; margin-left: 0px; table-layout: fixed; width: ";
    htmlTable += QString::number(width);
    if (relative) {
        htmlTable += "%";
    }
    else {
        htmlTable += "px";
    }
    htmlTable += ";\" ><tbody>";

    for(int i = 0; i < rows; ++i)
    {
        // Row header
        htmlTable += "<tr>";

        for(int j = 0; j < columns; ++j)
        {
            // Column header
            htmlTable += "<td style=\"border: 1px solid rgb(219, 219, 219); padding: 10 px; margin: 0px; width: ";
            htmlTable += QString::number(singleColumnWidth);
            if (relative) {
                htmlTable += "%";
            }
            else {
                htmlTable += "px";
            }
            htmlTable += ";\">";

            // Blank line to preserve the size
            htmlTable += "<div><br></div>";

            // End column
            htmlTable += "</td>";
        }

        // End row
        htmlTable += "</tr>";
    }

    // End table
    htmlTable += "</tbody></table></div>";
    return htmlTable;
}

void NoteEditorPrivate::attachResourceToNote(const QByteArray & data, const QString & dataHash, const QMimeType & mimeType)
{
    // TODO: implement
    Q_UNUSED(data)
    Q_UNUSED(dataHash)
    Q_UNUSED(mimeType)
}

void NoteEditorPrivate::insertImage(const QByteArray & data, const QString & dataHash, const QMimeType & mimeType)
{
    QNDEBUG("NoteEditorPrivate::insertImage: data hash = " << dataHash << ", mime type = " << mimeType.name());

    // Write the data to the temporary file
    // TODO: first check if there is the existing temporary file with this resource's data
    QTemporaryFile tmpFile;
    if (!tmpFile.open()) {
        QString error = QT_TR_NOOP("Can't render dropped image: can't create temporary file: ");
        error += tmpFile.errorString();
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    tmpFile.write(data);
    // TODO: put the path to the file to the local cache by resource's hash

    QString imageHtml = "<img src=\"file://";
    imageHtml += QFileInfo(tmpFile).absolutePath();
    imageHtml += "\" type=\"";
    imageHtml += mimeType.name();
    imageHtml += "\" hash=\"";
    // TODO: figure out how to deal with the context menu
    // TODO: idea: on hover can change cursor to "zoom" one and on LMB click can zoom the area of the image
    imageHtml += "\" />";

    execJavascriptCommand("insertHTML", imageHtml);
}

void NoteEditorPrivate::insertToDoCheckbox()
{
    QString html = ENMLConverter::getToDoCheckboxHtml(/* checked = */ false);
    execJavascriptCommand("insertHtml", html);
}

void NoteEditorPrivate::setFont(const QFont & font)
{
    QString fontName = font.family();
    execJavascriptCommand("fontName", fontName);
}

void NoteEditorPrivate::setFontHeight(const int height)
{
    if (height > 0) {
        execJavascriptCommand("fontSize", QString::number(height));
    }
    else {
        QString error = QT_TR_NOOP("Detected incorrect font size: " + QString::number(height));
        QNINFO(error);
        emit notifyError(error);
    }
}

void NoteEditorPrivate::setFontColor(const QColor & color)
{
    if (color.isValid()) {
        execJavascriptCommand("foreColor", color.name());
    }
    else {
        QString error = QT_TR_NOOP("Detected invalid font color: " + color.name());
        QNINFO(error);
        emit notifyError(error);
    }
}

void NoteEditorPrivate::setBackgroundColor(const QColor & color)
{
    if (color.isValid()) {
        execJavascriptCommand("hiliteColor", color.name());
    }
    else {
        QString error = QT_TR_NOOP("Detected invalid background color: " + color.name());
        QNINFO(error);
        emit notifyError(error);
    }
}

void NoteEditorPrivate::insertHorizontalLine()
{
    execJavascriptCommand("insertHorizontalRule");
}

void NoteEditorPrivate::changeIndentation(const bool increase)
{
    execJavascriptCommand((increase ? "indent" : "outdent"));
}

void NoteEditorPrivate::insertBulletedList()
{
    execJavascriptCommand("insertUnorderedList");
}

void NoteEditorPrivate::insertNumberedList()
{
    execJavascriptCommand("insertOrderedList");
}

#define CHECK_NUM_COLUMNS() \
    if (columns <= 0) { \
        QString error = QT_TR_NOOP("Detected attempt to insert table with bad number of columns: ") + QString::number(columns); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

#define CHECK_NUM_ROWS() \
    if (rows <= 0) { \
        QString error = QT_TR_NOOP("Detected attempt to insert table with bad number of rows: ") + QString::number(rows); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

void NoteEditorPrivate::insertFixedWidthTable(const int rows, const int columns, const int widthInPixels)
{
    CHECK_NUM_COLUMNS();
    CHECK_NUM_ROWS();

    Q_Q(NoteEditor);
    int pageWidth = q->geometry().width();
    if (widthInPixels > 2 * pageWidth) {
        QString error = QT_TR_NOOP("Can't insert table, width is too large (more than twice the page width): ") +
                        QString::number(widthInPixels);
        QNWARNING(error);
        emit notifyError(error);
        return;
    }
    else if (widthInPixels <= 0) {
        QString error = QT_TR_NOOP("Can't insert table, bad width: ") + QString::number(widthInPixels);
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    int singleColumnWidth = widthInPixels / columns;
    if (singleColumnWidth == 0) {
        QString error = QT_TR_NOOP("Can't insert table, bad width for specified number of columns "
                                   "(single column width is zero): width = ") +
                        QString::number(widthInPixels) + QT_TR_NOOP(", number of columns = ") +
                        QString::number(columns);
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QString htmlTable = composeHtmlTable(widthInPixels, singleColumnWidth, rows, columns,
                                         /* relative = */ false);
    execJavascriptCommand("insertHTML", htmlTable);
    updateColResizableTableBindings();
}

void NoteEditorPrivate::insertRelativeWidthTable(const int rows, const int columns, const double relativeWidth)
{
    CHECK_NUM_COLUMNS();
    CHECK_NUM_ROWS();

    if (relativeWidth <= 0.01) {
        QString error = QT_TR_NOOP("Can't insert table, relative width is too small: ") +
                        QString::number(relativeWidth) + QString("%");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }
    else if (relativeWidth > 100.0 +1.0e-9) {
        QString error = QT_TR_NOOP("Can't insert table, relative width is too large: ") +
                        QString::number(relativeWidth) + QString("%");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    double singleColumnWidth = relativeWidth / columns;
    QString htmlTable = composeHtmlTable(relativeWidth, singleColumnWidth, rows, columns,
                                         /* relative = */ true);
    execJavascriptCommand("insertHTML", htmlTable);
    updateColResizableTableBindings();
}

void NoteEditorPrivate::encryptSelectedText(const QString & passphrase,
                                            const QString & hint)
{
    QNDEBUG("NoteEditorPrivate::encryptSelectedText");

    Q_Q(NoteEditor);
    if (!q->page()->hasSelection()) {
        QString error = QT_TR_NOOP("Note editor page has no selected text, nothing to encrypt");
        QNINFO(error);
        emit notifyError(error);
        return;
    }

    // NOTE: use JavaScript to get the selected text here, not the convenience method of NoteEditorPage
    // in order to ensure the method returning the selected html and the method replacing the selected html
    // agree about the selected html
    QString selectedHtml = execJavascriptCommandWithResult("getSelectionHtml").toString();
    if (selectedHtml.isEmpty()) {
        QString error = QT_TR_NOOP("Selected html is empty, nothing to encrypt");
        QNINFO(error);
        emit notifyError(error);
        return;
    }

    QString error;
    QString encryptedText;
    QString cipher = "AES";
    size_t keyLength = 128;
    bool res = m_encryptionManager->encrypt(selectedHtml, passphrase, cipher, keyLength,
                                            encryptedText, error);
    if (!res) {
        error.prepend(QT_TR_NOOP("Can't encrypt selected text: "));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QString encryptedTextHtmlObject = "<object type=\"application/octet-stream\" en-tag=\"en-crypt\" >"
                                      "<param name=\"cipher\" value=\"";
    encryptedTextHtmlObject += cipher;
    encryptedTextHtmlObject += "\" />";
    encryptedTextHtmlObject += "<param name=\"length\" value=\"";
    encryptedTextHtmlObject += "\" />";
    encryptedTextHtmlObject += "<param name=\"encryptedText\" value=\"";
    encryptedTextHtmlObject += encryptedText;
    encryptedTextHtmlObject += "\" />";

    if (!hint.isEmpty()) {
        encryptedTextHtmlObject = "<param name=\"hint\" value=\"";
        encryptedTextHtmlObject += hint;    // FIXME: ensure there're no unescaped quotes and/or double-quotes here
        encryptedTextHtmlObject += "\" />";
    }

    encryptedTextHtmlObject += "<object/>";

    execJavascriptCommand("replaceSelectionWithHtml", encryptedTextHtmlObject);
    // TODO: ensure the contentChanged signal would be emitted automatically (guess it should)
}

void NoteEditorPrivate::dropFile(QString & filepath)
{
    QNDEBUG("NoteEditorPrivate::dropFile: " << filepath);

    QFileInfo fileInfo(filepath);
    if (!fileInfo.isFile()) {
        QNINFO("Detected attempt to drop something else rather than file: " << filepath);
        return;
    }

    if (!fileInfo.isReadable()) {
        QNINFO("Detected attempt to drop file which is not readable: " << filepath);
        return;
    }

    QMimeDatabase mimeDatabase;
    QMimeType mimeType = mimeDatabase.mimeTypeForFile(fileInfo);

    if (!mimeType.isValid()) {
        QNINFO("Detected invalid mime type for file " << filepath);
        return;
    }

    QByteArray data = QFile(filepath).readAll();
    QString dataHash = QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
    attachResourceToNote(data, dataHash, mimeType);
    // TODO: re-convert note's ENML to html and refresh the web page
}

} // namespace qute_note

void __initNoteEditorResources()
{
    Q_INIT_RESOURCE(checkbox_icons);
    Q_INIT_RESOURCE(generic_resource_icons);
    Q_INIT_RESOURCE(jquery);
    Q_INIT_RESOURCE(colResizable);
    Q_INIT_RESOURCE(scripts);

    QNDEBUG("Initialized NoteEditor's resources");
}
