#include "PageMutationHandler.h"

namespace quentier {

PageMutationHandler::PageMutationHandler(QObject * parent) :
    QObject(parent)
{}

void PageMutationHandler::onPageMutation()
{
    emit contentsChanged();
}

} // namespace quentier
