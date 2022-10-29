test_env = [
  'GIO_USE_VOLUME_MONITOR=unix',
  'GSETTINGS_BACKEND=memory',
  'G_ENABLE_DIAGNOSTIC=0',
  'G_TEST_SRCDIR=@0@'.format(meson.current_source_dir()),
  'G_TEST_BUILDDIR=@0@'.format(meson.current_build_dir()),
]

valgrind_tests = [
  'coordinate',
  'memory-cache',
  'viewport',
]

tests = [
  'file-cache',
  'marker',
  'map',
  'marker-layer',
]

if get_option('vector_renderer')
  valgrind_tests += [
    'vector-expression',
    'vector-style',
    'vector-value',
  ]
endif

subdir('data')

valgrind = find_program('valgrind', required: false)

if valgrind.found()
  foreach test : valgrind_tests
    executable = executable(
      test,
      test_resources,
      '@0@.c'.format(test),
      dependencies: [libshumate_dep],
    )

    test(test, valgrind, args: ['--leak-check=full', '--error-exitcode=1', executable], env: test_env)
  endforeach
else
  tests += valgrind_tests
endif

foreach test : tests
  executable = executable(
    test,
    test_resources,
    '@0@.c'.format(test),
    dependencies: [libshumate_dep],
  )

  test(test, executable, env: test_env)
endforeach