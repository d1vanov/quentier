#include "SharedNoteData.h"

namespace quentier {

SharedNoteData::SharedNoteData() :
    QSharedData(),
    m_noteGuid(),
    m_qecSharedNote(),
    m_indexInNote(-1)
{}

SharedNoteData::SharedNoteData(const SharedNoteData & other) :
    QSharedData(),
    m_noteGuid(other.m_noteGuid),
    m_qecSharedNote(other.m_qecSharedNote),
    m_indexInNote(other.m_indexInNote)
{}

SharedNoteData::SharedNoteData(SharedNoteData && other) :
    QSharedData(),
    m_noteGuid(std::move(other.m_noteGuid)),
    m_qecSharedNote(std::move(other.m_qecSharedNote)),
    m_indexInNote(std::move(other.m_indexInNote))
{}

SharedNoteData::SharedNoteData(const qevercloud::SharedNote & sharedNote) :
    QSharedData(),
    m_noteGuid(),
    m_qecSharedNote(sharedNote),
    m_indexInNote(-1)
{}

SharedNoteData::~SharedNoteData()
{}

} // namespace quentier
