#include "HTMLCleaner.h"
#include <logging/QuteNoteLogger.h>
#include <tidy.h>
#include <buffio.h>
#include <stdio.h>
#include <errno.h>

namespace qute_note {

class HTMLCleaner::Impl
{
public:
    Impl() :
        m_tidyOutput({0}),
        m_tidyErrorBuffer({0}),
        m_tidyDoc(tidyCreate())
    {}

    ~Impl()
    {
        tidyBufFree(&m_tidyOutput);
        tidyBufFree(&m_tidyErrorBuffer);
        tidyRelease(m_tidyDoc);
    }

    bool convertHtml(const QString & html, const TidyOptionId outputFormat,
                     QString & output);

    TidyBuffer  m_tidyOutput;
    TidyBuffer  m_tidyErrorBuffer;
    TidyDoc     m_tidyDoc;
};

HTMLCleaner::HTMLCleaner() :
    m_impl(new HTMLCleaner::Impl)
{}

HTMLCleaner::~HTMLCleaner()
{
    delete m_impl;
}

bool HTMLCleaner::htmlToXml(const QString & html, QString & output)
{
    QNDEBUG("HTMLCleaner::htmlToXml");
    QNTRACE("html = " << html);

    return m_impl->convertHtml(html, TidyXmlOut, output);
}

bool HTMLCleaner::htmlToXhtml(const QString & html, QString & output)
{
    QNDEBUG("HTMLCleaner::htmlToXhtml");
    QNTRACE("html = " << html);

    return m_impl->convertHtml(html, TidyXhtmlOut, output);
}

bool HTMLCleaner::cleanupHtml(QString & html)
{
    QNDEBUG("HTMLCleaner::cleanupHtml");
    QNTRACE("html = " << html);

    return m_impl->convertHtml(html, TidyHtmlOut, html);
}

bool HTMLCleaner::Impl::convertHtml(const QString & html, const TidyOptionId outputFormat, QString & output)
{
    int rc = -1;
    Bool ok = tidyOptSetBool(m_tidyDoc, outputFormat, yes);
    QNTRACE("tidyOptSetBool: ok = " << (ok ? "true" : "false"));
    if (ok) {
        rc = tidySetErrorBuffer(m_tidyDoc, &m_tidyErrorBuffer);
        QNTRACE("tidySetErrorBuffer: rc = " << rc);
    }

    if (rc >= 0) {
        rc = tidyParseString(m_tidyDoc, html.toLocal8Bit().constData());
        QNTRACE("tidyParseString: rc = " << rc);
    }

    if (rc >= 0) {
        rc = tidyCleanAndRepair(m_tidyDoc);
        QNTRACE("tidyCleanAndRepair: rc = " << rc);
    }

    if (rc >= 0) {
        rc = tidyRunDiagnostics(m_tidyDoc);
        QNTRACE("tidyRunDiagnostics: rc = " << rc);
    }

    if (rc > 1) {
        int forceOutputRc = tidyOptSetBool(m_tidyDoc, TidyForceOutput, yes);
        QNTRACE("tidyOptSetBool (force output): rc = " << forceOutputRc);
        if (!forceOutputRc) {
            rc = -1;
        }
    }

    if (rc >= 0) {
        rc = tidySaveBuffer(m_tidyDoc, &m_tidyOutput);
        QNTRACE("tidySaveBuffer: rc = " << rc);
    }

    if (rc >= 0)
    {
        if (rc > 0) {
            QNINFO("Tidy diagnostics: " << m_tidyErrorBuffer.bp);
        }

        output.clear();
        for(size_t i = 0; i < m_tidyOutput.size; ++i)
        {
            QString symbol = QString("%1").arg(m_tidyOutput.bp[i], 0, 16);
            // account for single-digit hex values (always must serialize as two digits)
            if(symbol.length() == 1) {
                output.append( "0" );
            }

            output.append(symbol);
        }

        return true;
    }

    return false;
}

}
