libshumate_public_h = [
  'shumate-bounding-box.h',
  'shumate-coordinate.h',
  'shumate-defines.h',
  'shumate-error-tile-source.h',
  'shumate-file-cache.h',
  'shumate-layer.h',
  'shumate-license.h',
  'shumate-location.h',
  'shumate-map-layer.h',
  'shumate-map-source-chain.h',
  'shumate-map-source-desc.h',
  'shumate-map-source-factory.h',
  'shumate-map-source.h',
  'shumate-marker-layer.h',
  'shumate-marker.h',
  'shumate-memory-cache.h',
  'shumate-network-tile-source.h',
  'shumate-path-layer.h',
  'shumate-point.h',
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
  'shumate-marker-private.h',
]

libshumate_sources = [
  'shumate-bounding-box.c',
  'shumate-coordinate.c',
  'shumate-debug.c',
  'shumate-error-tile-source.c',
  'shumate-file-cache.c',
  'shumate-layer.c',
  'shumate-license.c',
  'shumate-location.c',
  'shumate-map-layer.c',
  'shumate-map-source-chain.c',
  'shumate-map-source-desc.c',
  'shumate-map-source-factory.c',
  'shumate-map-source.c',
  'shumate-marker-layer.c',
  'shumate-marker.c',
  'shumate-memory-cache.c',
  'shumate-network-tile-source.c',
  'shumate-path-layer.c',
  'shumate-point.c',
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
  gtk_dep,
  cairo_dep,
  sqlite_dep,
  libsoup_dep,
]

libshumate_c_args = [
  '-DHAVE_CONFIG_H',
  '-DSHUMATE_COMPILATION',
  '-DG_LOG_DOMAIN="@0@"'.format(package_name),
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
  libshumate_features_h,
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
    extra_args: [
      '-DSHUMATE_COMPILATION',
    ]
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
