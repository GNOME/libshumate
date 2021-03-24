test_env = [
  'GIO_USE_VOLUME_MONITOR=unix',
  'GSETTINGS_BACKEND=memory',
  'G_ENABLE_DIAGNOSTIC=0',
  'G_TEST_SRCDIR=@0@'.format(meson.current_source_dir()),
  'G_TEST_BUILDDIR=@0@'.format(meson.current_build_dir()),
]

tests = [
  'coordinate',
  'file-cache',
  'marker',
  'view',
  'marker-layer',
  'network-tile-source',
  'viewport',
]

test_utils_sources = [
  'test-tile-server.c',
]
testutils_lib = shared_library(
  'testutils',
  test_utils_sources,
  dependencies: libshumate_dep,
)
testutils_dep = declare_dependency(
  link_with: testutils_lib,
  dependencies: libshumate_deps,
  sources: test_utils_sources,
)

foreach test : tests
  executable = executable(
    test,
    '@0@.c'.format(test),
    dependencies: [libshumate_dep, testutils_dep],
  )

  test(test, executable, env: test_env)
endforeach
