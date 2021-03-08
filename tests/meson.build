test_env = [
  'GIO_USE_VOLUME_MONITOR=unix',
  'GSETTINGS_BACKEND=memory',
  'G_ENABLE_DIAGNOSTIC=0',
  'G_TEST_SRCDIR=@0@'.format(meson.current_source_dir()),
  'G_TEST_BUILDDIR=@0@'.format(meson.current_build_dir()),
]

tests = [
  'coordinate',
  'marker',
  'view',
  'viewport',
]

foreach test : tests
  executable = executable(
    test,
    '@0@.c'.format(test),
    dependencies: libshumate_dep,
  )

  test(test, executable, env: test_env)
endforeach
