
#define FUSE_USE_VERSION 31

#define _GNU_SOURCE
#include "grpcpp/grpcpp.h"
#include "opentelemetry/exporters/otlp/otlp_grpc_exporter.h"
#include "opentelemetry/exporters/otlp/otlp_grpc_exporter_factory.h"

#include <fuse_lowlevel.h>
#include <string.h>
#include <stdio.h>

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

#include "workload_tracing.hpp"

namespace ot 		 = opentelemetry;
namespace trace_api      = ot::trace;
namespace trace_sdk      = ot::sdk::trace;
namespace trace_exporter = ot::exporter::trace;
namespace otlp 	 = ot::exporter::otlp;
namespace nostd = ot::nostd;
namespace resource = ot::sdk::resource;

static std::string serviceName = "MyService";
static std::string hostName = "MyHost";

static fuse_lowlevel_ops *tracing_next;

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

static void tracing_init(void *userdata, struct fuse_conn_info *conn)
{
	initTracer();	
	tracing_next->init(userdata, conn);
}

static void tracing_destroy(void *userdata) {
	cleanupTracer();
	tracing_next->destroy(userdata);
}

static void tracing_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                        struct fuse_file_info *fi) 
{
	auto provider = trace_api::Provider::GetTracerProvider();
	auto tracer = provider->GetTracer("Read", OPENTELEMETRY_SDK_VERSION);
	auto span = tracer->StartSpan("Test");
	//auto scope = trace_api::Scope(span);
	
	tracing_next->read(req, ino, size, off, fi);

	span->End();
}

fuse_lowlevel_ops tracing_operations(fuse_lowlevel_ops &next) {
	tracing_next = &next;

	fuse_lowlevel_ops curr = next;
	curr.init = tracing_init;
	curr.read = tracing_read;
	curr.destroy = tracing_destroy;
	return curr;
}