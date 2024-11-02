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

// Metrics with OTEL
std::shared_ptr<metric_api::MeterProvider> provider;
std::unique_ptr<metric_api::MeterProvider> prometheusProvider;
const std::string name = "fuse_otel_";

void initMetrics() {

	const std::string version = "1.2.0";
	const std::string schema = "https://opentelemetry.io/schemas/1.2.0";
	const std::string address = "localhost:8080";

	/*// OTLP GRPC exporter
	otlp::OtlpGrpcMetricExporterOptions options;
	options.endpoint = "localhost:4318"; // fix later

	auto exporter = otlp::OtlpGrpcMetricExporterFactory::Create(options);
	*/


	// Prometheus Exporter
	metric_exp::PrometheusExporterOptions promOpts;
	promOpts.url = address;
	auto exporter = metric_exp::PrometheusExporterFactory::Create(promOpts);

	// Initialize and set the global MeterProvider
	prometheusProvider = metric_sdk::MeterProviderFactory::Create();
	auto *p = static_cast<metric_sdk::MeterProvider *>(prometheusProvider.get());
	p->AddMetricReader(std::move(exporter));
	
	// Add read counter view
	std::string readCounterName = name + "read_counter";
	std::string readCounterUnits = "bytes";
	auto instrumentSelector = metric_sdk::InstrumentSelectorFactory::Create(
		metric_sdk::InstrumentType::kCounter, 
		readCounterName,
		readCounterUnits);
	auto meterSelector = metric_sdk::MeterSelectorFactory::Create(
		name, 
		version, 
		schema);
	auto sumView = metric_sdk::ViewFactory::Create(
		readCounterName, 
		"description", 
		readCounterUnits, 
		metric_sdk::AggregationType::kSum);
	p->AddView(std::move(instrumentSelector), std::move(meterSelector), std::move(sumView));

	provider = std::move(prometheusProvider);
	metric_api::Provider::SetMeterProvider(provider);
}

void cleanupMetrics() {
	std::shared_ptr<metric_api::MeterProvider> none;
	metric_api::Provider::SetMeterProvider(none);
}

nostd::unique_ptr<metric_api::Counter<uint64_t>> getReadCounter() {
	std::string readCounterName = name + "read_counter";
	auto provider = metric_api::Provider::GetMeterProvider();
	nostd::shared_ptr<metric_api::Meter> meter = provider->GetMeter(name, "1.2.0");
	auto counter = meter->CreateUInt64Counter(readCounterName);
	return counter;
}
