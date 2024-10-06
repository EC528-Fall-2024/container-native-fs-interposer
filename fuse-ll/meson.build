project('fuse_fs', 'c')

fuse = dependency('fuse3')

c = meson.get_compiler('c')
passthrough_ll_lib = c.find_library('passthrough_ll', dirs: [meson.current_source_dir() + '/passthrough_ll/target/release'])

executable(
    'passthrough',
    'passthrough.c',
    dependencies: [fuse, passthrough_ll_lib],
    install : true,
)
