project('fuse_fs', 'cpp')

fuse = dependency('fuse3')

# Protobuf dependency
proto_dep = dependency('protobuf')
grpc_dep = dependency('grpc++')

# Get OpenTelemetry dependency
ot_path = '/home/julia/source/install/lib'
ot_dep = dependency('opentelemetry-cpp',
  modules: [
    'opentelemetry-cpp::proto',
    'opentelemetry-cpp::api',
    'opentelemetry-cpp::trace',
    'opentelemetry-cpp::sdk',
    'opentelemetry-cpp::ostream_span_exporter',
    'opentelemetry-cpp::ext',
    'opentelemetry-cpp::version',
    'opentelemetry-cpp::common',
    'opentelemetry-cpp::otlp_grpc_exporter',
    'opentelemetry-cpp::otlp_grpc_client',
    'opentelemetry-cpp::otlp_recordable'
  ]
)


# Create executables

all_utility_fs = [
    ['basic_passthrough', []],
    ['workload_tracing', [ot_dep, proto_dep, grpc_dep]]
]

foreach utility_fs: all_utility_fs
    executable(
        utility_fs[0],
        utility_fs[0] + '.cpp',
        'passthrough/passthrough.cpp',
        'passthrough/passthrough.h',
        'passthrough/passthrough_helpers.h',
        dependencies: [fuse, utility_fs[1]],
        install : true,
    )
endforeach
