test_env = [
  'GIO_USE_VOLUME_MONITOR=unix',
  'GSETTINGS_BACKEND=memory',
  'G_ENABLE_DIAGNOSTIC=0',
  'G_TEST_SRCDIR=@0@'.format(meson.current_source_dir()),
  'G_TEST_BUILDDIR=@0@'.format(meson.current_build_dir()),
]

tests = {
  'coordinate': {},
  'data-source-request': {},
  'file-cache': { 'suite': 'no-valgrind' },
  'location': {},
  'map': { 'suite': 'no-valgrind' },
  'marker': { 'suite': 'no-valgrind' },
  'marker-layer': { 'suite': 'no-valgrind' },
  'memory-cache': {},
  'viewport': {},
}

if get_option('vector_renderer')
  tests += {
    'vector-expression': {},
    'vector-index': {},
    'vector-reader': {},
    'vector-renderer': {},
    'vector-sprite-sheet': {},
    'vector-style': {},
    'vector-value': {},
  }
endif

subdir('data')

# Allow the tests to be easily run under valgrind using --setup=valgrind
valgrind = find_program('valgrind', required: false)

if valgrind.found()
  add_test_setup('valgrind',
    exclude_suites: [ 'no-valgrind', 'flaky' ],
    exe_wrapper: [
      valgrind,
      '--leak-check=full',
      '--error-exitcode=1',
    ],
    env: test_env,
    timeout_multiplier: 20,
  )
endif

foreach test_name, args : tests
  executable = executable(
    test_name,
    test_resources,
    '@0@.c'.format(test_name),
    dependencies: [libshumate_dep],
    c_args: libshumate_c_args,
  )

  test(test_name, executable, env: test_env, suite: args.get('suite', []))
endforeach
