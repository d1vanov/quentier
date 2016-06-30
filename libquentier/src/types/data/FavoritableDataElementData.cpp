#include "FavoritableDataElementData.h"

namespace quentier {

FavoritableDataElementData::FavoritableDataElementData() :
    NoteStoreDataElementData(),
    m_isFavorited(false)
{}

FavoritableDataElementData::~FavoritableDataElementData()
{}

FavoritableDataElementData::FavoritableDataElementData(const FavoritableDataElementData & other) :
    NoteStoreDataElementData(other),
    m_isFavorited(other.m_isFavorited)
{}

FavoritableDataElementData::FavoritableDataElementData(FavoritableDataElementData && other) :
    NoteStoreDataElementData(std::move(other)),
    m_isFavorited(std::move(other.m_isFavorited))
{}

} // namespace quentier
