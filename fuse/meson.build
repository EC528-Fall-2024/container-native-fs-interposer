project('fuse_fs', 'c')

fuse = dependency('fuse3')

all_utility_fs = [
    'basic_passthrough',
    'workload_tracing'
]

foreach utility_fs: all_utility_fs
    executable(
        utility_fs,
        utility_fs + '.c',
        'passthrough/passthrough.c',
        'passthrough/passthrough.h',
        'passthrough/passthrough_helpers.h',
        dependencies: [fuse],
        install : true,
    )
endforeach