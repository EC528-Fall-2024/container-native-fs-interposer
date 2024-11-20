#include "otel.hpp"

std::string otlpEndpoint() {
    const char* endpoint = std::getenv("OTLP_ENDPOINT");
    if (endpoint)
        return std::string(endpoint);
    else
        return "localhost:4317";
}

void initTracer(std::string serviceName, std::string hostName, std::string endpt) {
	// Create OTLP exporter instance
	otlp::OtlpGrpcExporterOptions opts;
	opts.endpoint = endpt;
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

nostd::shared_ptr<trace_api::Tracer> getTracer(std::string libName) {
	auto provider = trace_api::Provider::GetTracerProvider();
	auto tracer = provider->GetTracer(libName, OPENTELEMETRY_SDK_VERSION);
    return tracer;
}

trace_api::Scope getScope(std::string libName, nostd::shared_ptr<trace_api::Span> span) {
    return getTracer(libName)->WithActiveSpan(span);
} 

nostd::shared_ptr<trace_api::Span> getSpan(std::string libName, std::string spanName) {
	//auto provider = trace_api::Provider::GetTracerProvider();
	//auto tracer = provider->GetTracer(libName, OPENTELEMETRY_SDK_VERSION);
    auto tracer = getTracer(libName);
    return tracer->StartSpan(spanName);
}

/*
nostd::shared_ptr<trace_api::Span> getSpan(
    std::string libName, 
    std::string spanName,
    trace_api::SpanContext context) {
	auto provider = trace_api::Provider::GetTracerProvider();
	auto tracer = provider->GetTracer(libName, OPENTELEMETRY_SDK_VERSION);
    return tracer->StartSpan(spanName, {{"context", context}});
}*/

// Metrics with OTEL
std::shared_ptr<metric_api::MeterProvider> provider;
const std::string name = "fuse_otel_";
const std::string version = "1.2.0";
const std::string schema = "https://opentelemetry.io/schemas/1.2.0";
const std::string address = "localhost:8080";

void initMetrics() {
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
	auto prometheusProvider = metric_sdk::MeterProviderFactory::Create();
	auto *p = static_cast<metric_sdk::MeterProvider *>(prometheusProvider.get());
	p->AddMetricReader(std::move(exporter));
	
	provider = std::move(prometheusProvider);
	metric_api::Provider::SetMeterProvider(provider);
}

void cleanupMetrics() {
	std::shared_ptr<metric_api::MeterProvider> none;
	metric_api::Provider::SetMeterProvider(none);
}

nostd::unique_ptr<metric_api::Counter<uint64_t>> getCounter(std::string counterName) {
	auto meterProvider = metric_api::Provider::GetMeterProvider();
	nostd::shared_ptr<metric_api::Meter> meter = meterProvider->GetMeter(name, version);
	auto counter = meter->CreateUInt64Counter(name + counterName);
	return counter;
}

nostd::unique_ptr<metric_api::Histogram<double>> getHistogram(
	std::string histName, 
	std::string description, 
	std::string unit) {
	
	auto meterProvider = metric_api::Provider::GetMeterProvider();
	nostd::shared_ptr<metric_api::Meter> meter = meterProvider->GetMeter(name, version);
	auto hist = meter->CreateDoubleHistogram(
		name + histName, 
		description, 
		unit);
	return hist;
}

nostd::unique_ptr<metric_api::UpDownCounter<int64_t>> getUpDownCounter(std::string counterName, std::string description, std::string unit) {
	auto meterProvider = metric_api::Provider::GetMeterProvider();
	nostd::shared_ptr<metric_api::Meter> meter = meterProvider->GetMeter(name, version);
	auto counter = meter->CreateInt64UpDownCounter(
		name + counterName,
		description,
		unit);
	return counter;
}

