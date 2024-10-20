#include "otel.hpp"

std::string otlpEndpoint() {
    const char* endpoint = std::getenv("OTLP_ENDPOINT");
    if (endpoint)
        return std::string(endpoint);
    else
        return "localhost:4317";
}

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

void initMetrics() {
	otlp::OtlpGrpcMetricExporterOptions options;
	options.endpoint = "localhost:4318"; // fix later

	auto exporter = otlp::OtlpGrpcMetricExporterFactory::Create(options);

	// Initialize and set the global MeterProvider
	metric_sdk::PeriodicExportingMetricReaderOptions reader_options;
	reader_options.export_interval_millis = std::chrono::milliseconds(1000);
	reader_options.export_timeout_millis  = std::chrono::milliseconds(500);

	auto reader =
		metric_sdk::PeriodicExportingMetricReaderFactory::Create(std::move(exporter), reader_options);

	auto context = metric_sdk::MeterContextFactory::Create();
	context->AddMetricReader(std::move(reader));

	auto u_provider = metric_sdk::MeterProviderFactory::Create(std::move(context));
	std::shared_ptr<opentelemetry::metrics::MeterProvider> provider(std::move(u_provider));

	metric_api::Provider::SetMeterProvider(provider);
}

void cleanupMetrics() {
	std::shared_ptr<metric_api::MeterProvider> none;
	metric_api::Provider::SetMeterProvider(none);
}
