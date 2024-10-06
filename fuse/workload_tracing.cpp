
#define FUSE_USE_VERSION 31

#define _GNU_SOURCE

#include <fuse.h>
#include <string.h>
#include <stdio.h>
#include "passthrough/passthrough.h"

#include "opentelemetry/trace/provider.h"
#include "opentelemetry/trace/tracer_provider.h"
#include "opentelemetry/sdk/trace/tracer.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/exporters/ostream/span_exporter.h"
#include "opentelemetry/sdk/trace/exporter.h"
#include "opentelemetry/sdk/trace/processor.h"
#include "opentelemetry/sdk/trace/simple_processor.h"
#include "opentelemetry/nostd/shared_ptr.h"
#include "opentelemetry/exporters/ostream/span_exporter_factory.h"
#include "opentelemetry/sdk/trace/simple_processor_factory.h"
#include "opentelemetry/sdk/trace/tracer_provider_factory.h"


namespace trace_api      = opentelemetry::trace;
namespace trace_sdk      = opentelemetry::sdk::trace;
namespace trace_exporter = opentelemetry::exporter::trace;

static void initTracer() {
  // Create ostream span exporter instance
  auto exporter  = trace_exporter::OStreamSpanExporterFactory::Create();
  auto processor = trace_sdk::SimpleSpanProcessorFactory::Create(std::move(exporter));

  std::shared_ptr<opentelemetry::sdk::trace::TracerProvider> sdk_provider =
      trace_sdk::TracerProviderFactory::Create(std::move(processor));

  // Set the global trace provider
  const std::shared_ptr<trace_api::TracerProvider> &api_provider = sdk_provider;
  trace_api::Provider::SetTracerProvider(api_provider);
}

static int tracing_read(const char *path, char *buf, size_t size, 
	off_t offset, struct fuse_file_info *fi) 
{
	auto provider = trace_api::Provider::GetTracerProvider();
	auto tracer = provider->GetTracer("Read");
	auto span = tracer->StartSpan("Reading..");
	span->SetAttribute("File path", path);

	int result = xmp_read(path, buf, size, offset, fi);

	span->End();
	return result;
}

// Workload tracing
int main(int argc, char *argv[])
{
	initTracer();

	// Replace function operations specific to workload tracing
	struct fuse_operations tracing_file_op = xmp_oper;
	tracing_file_op.read = tracing_read;

	enum { MAX_ARGS = 10 };
	int i,new_argc;
	char *new_argv[MAX_ARGS];

	umask(0);
			/* Process the "--plus" option apart */
	for (i=0, new_argc=0; (i<argc) && (new_argc<MAX_ARGS); i++) {
		if (!strcmp(argv[i], "--plus")) {
			fill_dir_plus = FUSE_FILL_DIR_PLUS;
		} else {
			new_argv[new_argc++] = argv[i];
		}
	}
	return fuse_main(new_argc, new_argv, &tracing_file_op, NULL);
}
