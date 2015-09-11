#include "PageMutationHandler.h"

namespace qute_note {

PageMutationHandler::PageMutationHandler(QObject * parent) :
    QObject(parent)
{}

void PageMutationHandler::onPageMutation()
{
    emit contentsChanged();
}

} // namespace qute_note
