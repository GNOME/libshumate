libshumate_public_h = [
  'shumate-adjustment.h',
  'shumate-bounding-box.h',
  'shumate-coordinate.h',
  'shumate-custom-marker.h',
  'shumate-defines.h',
  'shumate-error-tile-renderer.h',
  'shumate-exportable.h',
  'shumate-file-cache.h',
  'shumate-file-tile-source.h',
  'shumate-image-renderer.h',
  'shumate-kinetic-scroll-view.h',
  'shumate-label.h',
  'shumate-layer.h',
  'shumate-license.h',
  'shumate-location.h',
  'shumate-map-source-chain.h',
  'shumate-map-source-desc.h',
  'shumate-map-source-factory.h',
  'shumate-map-source.h',
  'shumate-marker-layer.h',
  'shumate-marker.h',
  'shumate-memory-cache.h',
  'shumate-network-bbox-tile-source.h',
  'shumate-network-tile-source.h',
  'shumate-null-tile-source.h',
  'shumate-path-layer.h',
  'shumate-point.h',
  'shumate-renderer.h',
  'shumate-scale.h',
  'shumate-tile-cache.h',
  'shumate-tile-source.h',
  'shumate-tile.h',
  'shumate-view.h',
  'shumate-viewport.h',
  'shumate.h',
]

libshumate_private_h = [
  'shumate-debug.h',
  'shumate-private.h',
]

libshumate_sources = [
  'shumate-adjustment.c',
  'shumate-bounding-box.c',
  'shumate-coordinate.c',
  'shumate-custom-marker.c',
  'shumate-debug.c',
  'shumate-error-tile-renderer.c',
  'shumate-exportable.c',
  'shumate-file-cache.c',
  'shumate-file-tile-source.c',
  'shumate-image-renderer.c',
  'shumate-kinetic-scroll-view.c',
  'shumate-label.c',
  'shumate-layer.c',
  'shumate-license.c',
  'shumate-location.c',
  'shumate-map-source-chain.c',
  'shumate-map-source-desc.c',
  'shumate-map-source-factory.c',
  'shumate-map-source.c',
  'shumate-marker-layer.c',
  'shumate-marker.c',
  'shumate-memory-cache.c',
  'shumate-network-bbox-tile-source.c',
  'shumate-network-tile-source.c',
  'shumate-null-tile-source.c',
  'shumate-path-layer.c',
  'shumate-point.c',
  'shumate-renderer.c',
  'shumate-scale.c',
  'shumate-tile-cache.c',
  'shumate-tile-source.c',
  'shumate-tile.c',
  'shumate-view.c',
  'shumate-viewport.c',
]

libshumate_deps = [
  libm_dep,
  glib_dep,
  gobject_dep,
  gio_dep,
  clutter_dep,
  cairo_dep,
  sqlite_dep,
  libsoup_dep,
]

libshumate_c_args = [
  '-DHAVE_CONFIG_H',
  '-DSHUMATE_COMPILATION',
  '-DG_LOG_DOMAIN="@0@"'.format(package_name),
]

libshumate_link_args = [
]

features_h = configuration_data()

libshumate_features_h = configure_file(
  output: 'shumate-features.h',
  configuration: features_h,
  install: true,
  install_dir: join_paths(
    pkgincludedir,
    'shumate',
  )
)

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
  libshumate_features_h,
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

libshumate_sha = shared_library(
  package_string,
  libshumate_sources,
  version: lib_version,
  include_directories: rootdir,
  dependencies: libshumate_deps,
  c_args: libshumate_c_args,
  link_args: libshumate_link_args,
  install: true,
  install_dir: libdir,
)

libshumate_dep_sources = [
  libshumate_enums_h,
  libshumate_features_h,
  libshumate_marshals_h,
  libshumate_version_h,
]

if generate_gir == true
  libshumate_gir_includes = [
    'GObject-2.0',
    'Clutter-1.0',
  ]

  libshumate_gir = gnome.generate_gir(
    libshumate_sha,
    sources: libshumate_sources + libshumate_public_h,
    nsversion: api_version,
    namespace: 'Shumate',
    symbol_prefix: 'shumate',
    identifier_prefix: 'Shumate',
    header: 'shumate/shumate.h',
    export_packages: [package_string],
    includes: libshumate_gir_includes,
    link_with: libshumate_sha,
    install: true,
    install_dir_gir: girdir,
    install_dir_typelib: typelibdir,
    extra_args: [
      '-DSHUMATE_COMPILATION',
    ]
  )

  libshumate_dep_sources += [
    libshumate_gir,
  ]

  if generate_vapi == true and libshumate_gir.length() > 0
    libshumate_vapi_packages = [
      'clutter-1.0',
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
  link_with: libshumate_sha,
  include_directories: rootdir,
  dependencies: libshumate_deps,
  sources: libshumate_dep_sources,
)

libshumate_pc = pkg.generate(
  name: package_string,
  description: 'Map View for Clutter',
  requires: ['glib-2.0', 'gobject-2.0', 'clutter-1.0'],
  requires_private: ['gio-2.0', 'cairo', 'libsoup-2.4', 'sqlite3'],
  version: version,
  subdirs: package_string,
  libraries: libshumate_sha,
  install_dir: pkgconfigdir,
)