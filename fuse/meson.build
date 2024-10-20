project('fuse_fs', 'cpp')

fuse = dependency('fuse3')

thread = dependency('threads')

# Protobuf dependency
proto_dep = dependency('protobuf')
grpc_dep = dependency('grpc++')

# Get OpenTelemetry dependency
ot_path = '/home/julia/source/install/lib'
ot_dep = dependency(
  'opentelemetry-cpp',
  modules: [
    'opentelemetry-cpp::proto',
    'opentelemetry-cpp::api',
    'opentelemetry-cpp::trace',
    'opentelemetry-cpp::metrics',
    'opentelemetry-cpp::sdk',
    'opentelemetry-cpp::ostream_span_exporter',
    'opentelemetry-cpp::ext',
    'opentelemetry-cpp::version',
    'opentelemetry-cpp::common',
    'opentelemetry-cpp::otlp_grpc_exporter',
    'opentelemetry-cpp::otlp_grpc_client',
    'opentelemetry-cpp::otlp_recordable',
    'opentelemetry-cpp::otlp_grpc_metrics_exporter'
  ]
)

# Create executables

executable(
  'interposer',
  'interposer.cc',
  'include/passthrough_hp.cc',
  'include/otel.cpp',
  'include/workload_tracing.cpp',
  'include/metric_collection.cpp',
  dependencies: [thread, fuse, ot_dep, proto_dep, grpc_dep],
  install: true
)
