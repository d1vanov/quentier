#include "Notebook.h"

namespace qute_note {

Notebook::Notebook() :
    NoteStoreDataElement()
{}

Notebook::Notebook(const Notebook &other) :
    NoteStoreDataElement(other),
    m_enNotebook(other.m_enNotebook)
{}

Notebook & Notebook::operator=(const Notebook & other)
{
    if (this != &other) {
        NoteStoreDataElement::operator=(other);
        m_enNotebook = other.m_enNotebook;
    }

    return *this;
}

Notebook::~Notebook()
{}

void Notebook::clear()
{
    m_enNotebook = evernote::edam::Notebook();
}

bool Notebook::hasGuid() const
{
    return m_enNotebook.__isset.guid;
}

const QString Notebook::guid() const
{
    return std::move(QString::fromStdString(m_enNotebook.guid));
}

void Notebook::setGuid(const QString & guid)
{
    m_enNotebook.guid = guid.toStdString();
    m_enNotebook.__isset.guid = !guid.isEmpty();
}

bool Notebook::hasUpdateSequenceNumber() const
{
    return m_enNotebook.__isset.updateSequenceNum;
}

qint32 Notebook::updateSequenceNumber() const
{
    return static_cast<qint32>(m_enNotebook.updateSequenceNum);
}

void Notebook::setUpdateSequenceNumber(const qint32 usn)
{
    m_enNotebook.updateSequenceNum = static_cast<int32_t>(usn);
    m_enNotebook.__isset.updateSequenceNum = true;
}

bool Notebook::checkParameters(QString & errorDescription) const
{
    Q_UNUSED(errorDescription);

    // TODO: implement
    return true;
}

QTextStream & Notebook::Print(QTextStream & strm) const
{
    // TODO: implement
    return strm;
}



} // namespace qute_note
