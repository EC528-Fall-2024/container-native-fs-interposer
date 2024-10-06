project('fuse_fs', 'cpp')

fuse = dependency('fuse3')

# Get OpenTelemetry dependency
ot_path = '/home/julia/source/install/lib'
ot_dep = dependency('opentelemetry-cpp',
  modules: [
    'opentelemetry-cpp::api',
    'opentelemetry-cpp::trace',
    'opentelemetry-cpp::sdk',
    'opentelemetry-cpp::ostream_span_exporter',
    'opentelemetry-cpp::ext',
    'opentelemetry-cpp::version',
    'opentelemetry-cpp::common'
  ]
)

# Create executables

all_utility_fs = [
    ['basic_passthrough', []],
    ['workload_tracing', [ot_dep]]
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
