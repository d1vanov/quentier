#ifndef QUENTIER_COMMAND_LINE_PARSER_H
#define QUENTIER_COMMAND_LINE_PARSER_H

#include <quentier/utility/Macros.h>
#include <quentier/types/ErrorString.h>
#include <QString>
#include <QHash>

namespace quentier {

class CommandLineParser
{
public:
    explicit CommandLineParser(int argc, char * argv[]);

    QString responseMessage() const;
    bool shouldQuit() const;

    bool hasError() const;
    ErrorString errorDescription() const;

    typedef QHash<QString, QVariant> CommandLineOptions;
    CommandLineOptions options() const;

private:
    Q_DISABLE_COPY(CommandLineParser)

private:
    QString             m_responseMessage;
    bool                m_shouldQuit;
    ErrorString         m_errorDescription;
    CommandLineOptions  m_parsedArgs;
};

} // namespace quentier

#endif // QUENTIER_COMMAND_LINE_PARSER_H
