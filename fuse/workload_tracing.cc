#define FUSE_USE_VERSION FUSE_MAKE_VERSION(3, 12)

#define _GNU_SOURCE

#include <fuse_lowlevel.h>
#include <stdio.h>
#include <string.h>

#include "opentelemetry/exporters/ostream/span_exporter.h"
#include "opentelemetry/exporters/ostream/span_exporter_factory.h"
#include "opentelemetry/nostd/shared_ptr.h"
#include "opentelemetry/sdk/trace/exporter.h"
#include "opentelemetry/sdk/trace/processor.h"
#include "opentelemetry/sdk/trace/simple_processor.h"
#include "opentelemetry/sdk/trace/simple_processor_factory.h"
#include "opentelemetry/sdk/trace/tracer.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/sdk/trace/tracer_provider_factory.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/trace/tracer_provider.h"

#include "workload_tracing.h"

namespace trace_api = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;
namespace trace_exporter = opentelemetry::exporter::trace;

static fuse_lowlevel_ops *tracing_next;
static opentelemetry::v1::nostd::shared_ptr<
    opentelemetry::v1::trace::TracerProvider>
    provider;

static void tracing_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                         struct fuse_file_info *fi) {
  auto tracer = provider->GetTracer("read");
  auto span = tracer->StartSpan("Reading..");
  // span->SetAttribute("File path", path);

  tracing_next->read(req, ino, size, off, fi);

  span->End();
}

static void tracing_write_buf(fuse_req_t req, fuse_ino_t ino,
                              struct fuse_bufvec *bufv, off_t off,
                              struct fuse_file_info *fi) {
  auto tracer = provider->GetTracer("write_buf");
  auto span = tracer->StartSpan("Writing..");

  tracing_next->write_buf(req, ino, bufv, off, fi);

  span->End();
}

fuse_lowlevel_ops tracing_operations(fuse_lowlevel_ops &next) {
  // Create ostream span exporter instance
  auto exporter = trace_exporter::OStreamSpanExporterFactory::Create();
  auto processor =
      trace_sdk::SimpleSpanProcessorFactory::Create(std::move(exporter));

  std::shared_ptr<opentelemetry::sdk::trace::TracerProvider> sdk_provider =
      trace_sdk::TracerProviderFactory::Create(std::move(processor));

  // Set the global trace provider
  const std::shared_ptr<trace_api::TracerProvider> &api_provider = sdk_provider;
  trace_api::Provider::SetTracerProvider(api_provider);

  provider = trace_api::Provider::GetTracerProvider();

  tracing_next = &next;

  fuse_lowlevel_ops curr = next;

  curr.read = tracing_read;
  curr.write_buf = tracing_write_buf;

  return curr;
}
