#ifndef OTEL_HPP_INCLUDED
#define OTEL_HPP_INCLUDED


#include "grpcpp/grpcpp.h"
#include "opentelemetry/exporters/otlp/otlp_grpc_exporter.h"
#include "opentelemetry/exporters/otlp/otlp_grpc_exporter_factory.h"
#include "opentelemetry/exporters/otlp/otlp_grpc_exporter_options.h"
#include "opentelemetry/exporters/ostream/span_exporter_factory.h"
#include "opentelemetry/exporters/ostream/span_exporter.h"
#include "opentelemetry/nostd/shared_ptr.h"
#include "opentelemetry/sdk/resource/semantic_conventions.h"
#include "opentelemetry/sdk/trace/exporter.h"
#include "opentelemetry/sdk/trace/processor.h"
#include "opentelemetry/sdk/trace/simple_processor_factory.h"
#include "opentelemetry/sdk/trace/simple_processor.h"
#include "opentelemetry/sdk/trace/tracer_provider_factory.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/sdk/trace/tracer.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/trace/scope.h"
#include "opentelemetry/trace/span.h"
#include "opentelemetry/trace/tracer_provider.h"

namespace ot 		     = opentelemetry;
namespace trace_api      = ot::trace;
namespace trace_sdk      = ot::sdk::trace;
namespace trace_exporter = ot::exporter::trace;
namespace otlp 	         = ot::exporter::otlp;
namespace nostd          = ot::nostd;
namespace resource       = ot::sdk::resource;

std::string otlpEndpoint() {
    const char* endpoint = std::getenv("OTLP_ENDPOINT");
    if (endpoint)
        return std::string(endpoint);
    else
        return "localhost:4317";
}

// Tracing helper functions

void initTracer(std::string serviceName, std::string hostName) {
	// Create OTLP exporter instance
	otlp::OtlpGrpcExporterOptions opts;
	opts.endpoint = otlpEndpoint();
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

void cleanupTracer() {
	std::shared_ptr<trace_api::TracerProvider> none;
	trace_api::Provider::SetTracerProvider(none);
}

nostd::shared_ptr<trace_api::Span> getSpan(std::string libName, std::string spanName) {
	auto provider = trace_api::Provider::GetTracerProvider();
	auto tracer = provider->GetTracer(libName, OPENTELEMETRY_SDK_VERSION);
    return tracer->StartSpan(spanName);
}


#endif // OTEL_HPP_INCLUDED