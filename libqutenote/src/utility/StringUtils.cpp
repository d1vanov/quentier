#include <qute_note/utility/StringUtils.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <QRegExp>

namespace qute_note {

void removePunctuation(QString & str, const bool keepAsterisk)
{
    QNDEBUG("removePunctuation: " << str << "; keep asterisk = " << (keepAsterisk ? "true" : "false"));

    QString filterStr = QString::fromUtf8("[`~!@#$%^&()—+=|:;<>«»,.?/{}\'\"\\[\\]]");
    if (!keepAsterisk) {
        filterStr += "*";
    }

    QRegExp punctuationFilter(filterStr);
    str.remove(punctuationFilter);

    QNTRACE("After removing punctuation: " << str);
}

} // namespace qute_note
