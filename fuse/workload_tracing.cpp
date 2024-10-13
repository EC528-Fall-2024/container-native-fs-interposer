
#define FUSE_USE_VERSION 31

#define _GNU_SOURCE
#include "grpcpp/grpcpp.h"
#include "opentelemetry/exporters/otlp/otlp_grpc_exporter.h"
#include "opentelemetry/exporters/otlp/otlp_grpc_exporter_factory.h"

#include <fuse.h>
#include <string.h>
#include <stdio.h>
#include "passthrough/passthrough.h"

#include "opentelemetry/nostd/shared_ptr.h"
#include "opentelemetry/trace/scope.h"
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
#include <iostream>
#include <fstream>
#include "opentelemetry/exporters/otlp/otlp_grpc_exporter_options.h"
#include "opentelemetry/sdk/resource/semantic_conventions.h"


namespace ot 		 = opentelemetry;
namespace trace_api      = ot::trace;
namespace trace_sdk      = ot::sdk::trace;
namespace trace_exporter = ot::exporter::trace;
namespace otlp 	 = ot::exporter::otlp;
namespace nostd = ot::nostd;
namespace resource = ot::sdk::resource;

static std::string serviceName = "MyService";
static std::string hostName = "MyHost";


static void initTracer() {
	// Create OTLP exporter instance
	otlp::OtlpGrpcExporterOptions opts;
	opts.endpoint = "localhost:4317";
	auto exporter = otlp::OtlpGrpcExporterFactory::Create(opts);
	auto processor = std::unique_ptr<trace_sdk::SpanProcessor>(
		new trace_sdk::SimpleSpanProcessor(std::move(exporter))
	);

	resource::ResourceAttributes attributes = {
		{resource::SemanticConventions::kServiceName, serviceName},
		{resource::SemanticConventions::kHostName, hostName}
	};
	auto resource = resource::Resource::Create(attributes);

	std::shared_ptr<trace_api::TracerProvider> provider 
		= trace_sdk::TracerProviderFactory::Create(std::move(processor), std::move(resource));

	// Set global trace provider
	trace_api::Provider::SetTracerProvider(provider);
}

static void cleanupTracer() {
	std::shared_ptr<trace_api::TracerProvider> none;
	trace_api::Provider::SetTracerProvider(none);
}

static void tracing_destroy(void *private_data) {
	cleanupTracer();
}

static int tracing_read(const char *path, char *buf, size_t size, 
	off_t offset, struct fuse_file_info *fi) 
{
	auto provider = trace_api::Provider::GetTracerProvider();
	
	auto tracer = provider->GetTracer("ReadTracer", OPENTELEMETRY_SDK_VERSION);
	if (!tracer) {
		return 1;
	}
	//auto span = trace_api::Scope(tracer->StartSpan("Mytest"));
	//span->SetAttribute("File path", path);
	//span->SetAttribute("Size", size);
	auto span = tracer->StartSpan("REadTest");
	//auto scope = trace_api::Scope(span);
	span->End();
	int result = xmp_read(path, buf, size, offset, fi);
	return result;
}

/*
static int tracing_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	auto provider = trace_api::Provider::GetTracerProvider();
        auto tracer = provider->GetTracer("Write");
	auto span = tracer->StartSpan("Write Operation");
        span->SetAttribute("File path", path);
        span->SetAttribute("Size", size);

        int result = xmp_write(path, buf, size, offset, fi);

        span->End();
        return result;

}

static int tracing_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi,
		       enum fuse_readdir_flags flags)
{
	auto provider = trace_api::Provider::GetTracerProvider();
        auto tracer = provider->GetTracer("Read directory", OPENTELEMETRY_SDK_VERSION);
        auto span = tracer->StartSpan("Read directory operation");
        span->SetAttribute("File path", path);
	

        int result = xmp_readdir(path, buf, filler, offset, fi, flags);

        span->End();
        return result;
}
*/

static void *tracing_init(struct fuse_conn_info *conn,
		      struct fuse_config *cfg)
{
	initTracer();	
	void * result = xmp_init(conn, cfg);
	return result;
}

// Workload tracing
int main(int argc, char *argv[])
{
	// Replace function operations specific to workload tracing
	struct fuse_operations tracing_file_op = xmp_oper;
	tracing_file_op.init = tracing_init;
	tracing_file_op.read = tracing_read;
	tracing_file_op.destroy = tracing_destroy;
	//tracing_file_op.write = tracing_write;
	//tracing_file_op.readdir = tracing_readdir;
	
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
