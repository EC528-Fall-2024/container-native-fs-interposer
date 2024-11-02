#ifndef OTEL_HPP_INCLUDED
#define OTEL_HPP_INCLUDED


#include "grpcpp/grpcpp.h"
#include "opentelemetry/exporters/otlp/otlp_grpc_exporter.h"
#include "opentelemetry/exporters/otlp/otlp_grpc_exporter_factory.h"
#include "opentelemetry/exporters/otlp/otlp_grpc_metric_exporter_factory.h"
#include "opentelemetry/exporters/otlp/otlp_grpc_exporter_options.h"
#include "opentelemetry/exporters/ostream/span_exporter_factory.h"
#include "opentelemetry/exporters/ostream/span_exporter.h"
#include "opentelemetry/metrics/provider.h"
#include "opentelemetry/nostd/shared_ptr.h"
#include "opentelemetry/sdk/metrics/aggregation/default_aggregation.h"
#include "opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader.h"
#include "opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader_factory.h"
#include "opentelemetry/sdk/metrics/meter.h"
#include "opentelemetry/sdk/metrics/meter_context_factory.h"
#include "opentelemetry/sdk/metrics/meter_provider.h"
#include "opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader.h"
#include "opentelemetry/sdk/metrics/meter_provider_factory.h"
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
#include "opentelemetry/exporters/ostream/metric_exporter_factory.h"
#include "opentelemetry/exporters/prometheus/exporter.h"
#include "opentelemetry/exporters/prometheus/exporter_factory.h"
#include "opentelemetry/exporters/prometheus/exporter_options.h"
#include "opentelemetry/exporters/prometheus/collector.h"
#include "opentelemetry/exporters/prometheus/exporter_utils.h"
#include "prometheus/exposer.h"
#include "opentelemetry/sdk/metrics/aggregation/default_aggregation.h"
#include "opentelemetry/sdk/metrics/aggregation/histogram_aggregation.h"
#include "opentelemetry/sdk/metrics/view/instrument_selector_factory.h"
#include "opentelemetry/sdk/metrics/view/meter_selector_factory.h"
#include "opentelemetry/sdk/metrics/view/view_factory.h"

namespace ot 		     = opentelemetry;
namespace common		 = ot::common;
namespace trace_api      = ot::trace;
namespace trace_sdk      = ot::sdk::trace;
namespace trace_exporter = ot::exporter::trace;
namespace otlp 	         = ot::exporter::otlp;
namespace nostd          = ot::nostd;
namespace resource       = ot::sdk::resource;
namespace metric_api	 = ot::metrics;
namespace metric_sdk	 = ot::sdk::metrics;
namespace metric_exp     = ot::exporter::metrics;

std::string otlpEndpoint();

// Tracing helper functions
void initTracer(std::string serviceName, std::string hostName);
void cleanupTracer();
nostd::shared_ptr<trace_api::Span> getSpan(std::string libName, std::string spanName);

// Metrics helper functions
void initMetrics();
void cleanupMetrics();
nostd::unique_ptr<metric_api::Counter<uint64_t>> getReadCounter();


#endif // OTEL_HPP_INCLUDED
