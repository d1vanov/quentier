#include "TProcessor.h"

namespace apache { namespace thrift {

TProcessorEventHandler::~TProcessorEventHandler()
{}

void* TProcessorEventHandler::getContext(const char *fn_name, void *serverContext)
{
    (void) fn_name;
    (void) serverContext;
    return NULL;
}

void TProcessorEventHandler::freeContext(void *ctx, const char *fn_name)
{
    (void) ctx;
    (void) fn_name;
}

void TProcessorEventHandler::preRead(void* ctx, const char* fn_name) {
  (void) ctx;
  (void) fn_name;
}

void TProcessorEventHandler::postRead(void* ctx, const char* fn_name,
                                      uint32_t bytes) {
  (void) ctx;
  (void) fn_name;
  (void) bytes;
}

void TProcessorEventHandler::preWrite(void* ctx, const char* fn_name) {
  (void) ctx;
  (void) fn_name;
}

void TProcessorEventHandler::postWrite(void* ctx, const char* fn_name,
                                       uint32_t bytes) {
    (void) ctx;
    (void) fn_name;
    (void) bytes;
}

void TProcessorEventHandler::asyncComplete(void* ctx, const char* fn_name) {
    (void) ctx;
    (void) fn_name;
}

void TProcessorEventHandler::handlerError(void* ctx, const char* fn_name) {
    (void) ctx;
    (void) fn_name;
}

}}
