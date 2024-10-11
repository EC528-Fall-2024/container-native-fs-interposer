project('interposer', 'cpp')

cxxopts = dependency('cxxopts')
fuse = dependency('fuse3')
opentelemetry = dependency(
    'opentelemetry-cpp',
    modules: [
        'opentelemetry-cpp::api',
        'opentelemetry-cpp::trace',
        'opentelemetry-cpp::sdk',
        'opentelemetry-cpp::ostream_span_exporter',
        'opentelemetry-cpp::ext',
        'opentelemetry-cpp::version',
        'opentelemetry-cpp::common',
    ],
)

executable(
    'interposer',
    'passthrough_hp.cc',
    'workload_tracing.cc',
    'faulty_delay.cc',
    dependencies: [cxxopts, fuse, opentelemetry],
    install: true,
)
