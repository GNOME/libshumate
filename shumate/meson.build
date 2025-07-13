libshumate_public_h = [
  'shumate-coordinate.h',
  'shumate-compass.h',
  'shumate-data-source.h',
  'shumate-data-source-request.h',
  'shumate-file-cache.h',
  'shumate-layer.h',
  'shumate-license.h',
  'shumate-location.h',
  'shumate-map.h',
  'shumate-map-layer.h',
  'shumate-map-source-registry.h',
  'shumate-map-source.h',
  'shumate-marker-layer.h',
  'shumate-marker.h',
  'shumate-path-layer.h',
  'shumate-point.h',
  'shumate-scale.h',
  'shumate-simple-map.h',
  'shumate-symbol-event.h',
  'shumate-raster-renderer.h',
  'shumate-tile.h',
  'shumate-tile-downloader.h',
  'shumate-user-agent.h',
  'shumate-vector-reader.h',
  'shumate-vector-reader-iter.h',
  'shumate-vector-renderer.h',
  'shumate-vector-sprite.h',
  'shumate-vector-sprite-sheet.h',
  'shumate-viewport.h',
  'shumate.h',
]

libshumate_private_h = [
  'shumate-inspector-page-private.h',
  'shumate-inspector-settings-private.h',
  'shumate-kinetic-scrolling-private.h',
  'shumate-marker-private.h',
  'shumate-memory-cache-private.h',
  'shumate-profiling-private.h',
  'shumate-symbol-event-private.h',
  'shumate-tile-private.h',
  'shumate-utils-private.h',
  'shumate-vector-reader-private.h',
  'shumate-vector-reader-iter-private.h',
  'shumate-viewport-private.h',

  'vector/shumate-vector-background-layer-private.h',
  'vector/shumate-vector-collision-private.h',
  'vector/shumate-vector-expression-private.h',
  'vector/shumate-vector-expression-filter-private.h',
  'vector/shumate-vector-expression-interpolate-private.h',
  'vector/shumate-vector-fill-layer-private.h',
  'vector/shumate-vector-index-private.h',
  'vector/shumate-vector-layer-private.h',
  'vector/shumate-vector-line-layer-private.h',
  'vector/shumate-vector-render-scope-private.h',
  'vector/shumate-vector-symbol-private.h',
  'vector/shumate-vector-symbol-container-private.h',
  'vector/shumate-vector-symbol-info-private.h',
  'vector/shumate-vector-symbol-layer-private.h',
  'vector/shumate-vector-utils-private.h',
  'vector/shumate-vector-value-private.h',
  'vector/vector_tile.pb-c.h',
]

libshumate_sources = [
  'shumate-coordinate.c',
  'shumate-data-source.c',
  'shumate-data-source-request.c',
  'shumate-compass.c',
  'shumate-file-cache.c',
  'shumate-inspector-page.c',
  'shumate-inspector-settings.c',
  'shumate-kinetic-scrolling.c',
  'shumate-layer.c',
  'shumate-license.c',
  'shumate-location.c',
  'shumate-map.c',
  'shumate-map-layer.c',
  'shumate-map-source-registry.c',
  'shumate-map-source.c',
  'shumate-marker-layer.c',
  'shumate-marker.c',
  'shumate-memory-cache.c',
  'shumate-path-layer.c',
  'shumate-point.c',
  'shumate-scale.c',
  'shumate-simple-map.c',
  'shumate-symbol-event.c',
  'shumate-raster-renderer.c',
  'shumate-tile.c',
  'shumate-tile-downloader.c',
  'shumate-user-agent.c',
  'shumate-utils.c',
  'shumate-vector-reader.c',
  'shumate-vector-reader-iter.c',
  'shumate-vector-renderer.c',
  'shumate-vector-sprite.c',
  'shumate-vector-sprite-sheet.c',
  'shumate-viewport.c',
]

libshumate_deps = [
  libm_dep,
  glib_dep,
  gobject_dep,
  gio_dep,
  gtk_dep,
  cairo_dep,
  sqlite_dep,
  libsoup_dep,
  libsysprof_capture_dep,
]

libshumate_c_args = [
  '-DHAVE_CONFIG_H',
  '-DSHUMATE_COMPILATION',
  '-DG_LOG_DOMAIN="@0@"'.format(package_name),
]

if not get_option('buildtype').startswith('debug')
  libshumate_c_args += [
    '-DG_DISABLE_ASSERT',
  ]
endif

version_h = configuration_data()
version_h.set('SHUMATE_MAJOR_VERSION', version_major)
version_h.set('SHUMATE_MINOR_VERSION', version_minor)
version_h.set('SHUMATE_MICRO_VERSION', version_micro)
version_h.set('SHUMATE_VERSION', version)

libshumate_version_h = configure_file(
  input: 'shumate-version.h.in',
  output: 'shumate-version.h',
  configuration: version_h,
  install: true,
  install_dir: join_paths(
    pkgincludedir,
    'shumate',
  )
)

libshumate_public_h += [
  libshumate_version_h,
]

libshumate_marshals = gnome.genmarshal(
  'shumate-marshal',
  sources: 'shumate-marshal.list',
  prefix: '_shumate_marshal',
  install_header: true,
  install_dir: join_paths(
    pkgincludedir,
    'shumate',
  )
)

libshumate_enums = gnome.mkenums(
  'shumate-enum-types',
  sources: libshumate_public_h,
  h_template: 'shumate-enum-types.h.in',
  c_template: 'shumate-enum-types.c.in',
  install_header: true,
  install_dir: join_paths(
    pkgincludedir,
    'shumate',
  )
)

libshumate_marshals_h = libshumate_marshals.get(1)
libshumate_enums_h = libshumate_enums.get(1)

libshumate_sources += [
  libshumate_marshals,
  libshumate_enums,
]

install_headers(
  libshumate_public_h,
  install_dir: join_paths(
    pkgincludedir,
    'shumate',
  )
)

if get_option('vector_renderer')
  subdir('vector')
endif

libshumate_lib = library(
  package_string,
  libshumate_sources,
  shumate_resources,
  version: lib_version,
  include_directories: rootdir,
  dependencies: libshumate_deps,
  c_args: libshumate_c_args,
  install: true,
  install_dir: libdir,
)

libshumate_dep_sources = [
  libshumate_enums_h,
  libshumate_marshals_h,
  libshumate_version_h,
]

if generate_gir == true
  libshumate_gir_includes = [
    'GObject-2.0',
    'Gdk-4.0',
    'Gtk-4.0',
    'Gsk-4.0',
  ]
  libshumate_gir_extra_args = [
    '-DSHUMATE_COMPILATION',
  ]
  if gi_scanner_dep.version().version_compare('>=1.83.2')
    libshumate_gir_extra_args += [
      '--doc-format=gi-docgen',
    ]
  endif

  libshumate_gir = gnome.generate_gir(
    libshumate_lib,
    sources: libshumate_sources + libshumate_public_h,
    nsversion: api_version,
    namespace: 'Shumate',
    symbol_prefix: 'shumate',
    identifier_prefix: 'Shumate',
    header: 'shumate/shumate.h',
    export_packages: [package_string],
    includes: libshumate_gir_includes,
    install: true,
    install_dir_gir: girdir,
    install_dir_typelib: typelibdir,
    fatal_warnings: true,
    extra_args: libshumate_gir_extra_args,
  )

  libshumate_dep_sources += [
    libshumate_gir,
    shumate_resources,
  ]

  if generate_vapi == true and libshumate_gir.length() > 0
    libshumate_vapi_packages = [
      'gio-2.0',
      'gtk4',
      'cogl-pango-1.0',
      'atk',
      'pangocairo',
    ]

    libshumate_vapi = gnome.generate_vapi(
      package_string,
      sources: libshumate_gir.get(0),
      packages: libshumate_vapi_packages,
      install: true,
      install_dir: vapidir,
    )
  endif
endif

libshumate_dep = declare_dependency(
  link_with: libshumate_lib,
  include_directories: rootdir,
  dependencies: libshumate_deps,
  sources: libshumate_dep_sources,
)

libshumate_pc = pkg.generate(
  libshumate_lib,
  name: package_string,
  description: 'Map View for GTK',
  requires: [glib_dep, gobject_dep, gtk_dep],
  subdirs: package_string,
  install_dir: pkgconfigdir,
)
