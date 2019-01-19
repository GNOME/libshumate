project(
  'libshumate', 'c',
  version: '0.0.0',
  license: 'LGPLv2.1+',
  meson_version: '>= 0.46.1',
  default_options: [
    'buildtype=debugoptimized',
  ]
)

gnome = import('gnome')
pkg = import('pkgconfig')

# Versioning
version = meson.project_version()
version_arr = version.split('.')
version_major = version_arr[0]
version_minor = version_arr[1]
version_micro = version_arr[2]

if version_minor.to_int().is_odd()
  api_version = '.'.join([version_major, version_minor + 1])
else
  api_version = '.'.join([version_major, version_minor])
endif

lib_version = '0.0'

package_name = meson.project_name().strip('lib')
package_string = '@0@-@1@'.format(package_name, api_version)

# Paths
rootdir = include_directories('.')

srcdir = meson.current_source_dir()
builddir = meson.current_build_dir()

prefix = get_option('prefix')

includedir = join_paths(prefix, get_option('includedir'))
libdir = join_paths(prefix, get_option('libdir'))
datadir = join_paths(prefix, get_option('datadir'))
pkgincludedir = join_paths(includedir, package_string)
typelibdir = join_paths(libdir, 'girepository-1.0')
pkgconfigdir = join_paths(libdir, 'pkgconfig')

girdir = join_paths(datadir, 'gir-1.0')
gtkdocdir = join_paths(datadir, 'gtk-doc')
vapidir = join_paths(datadir, 'vala', 'vapi')

# Dependencies
cc = meson.get_compiler('c')

libm_dep = cc.find_library('m', required: true)

glib_req = '>= 2.16.0'
clutter_req = '>= 1.12'
cairo_req = '>= 1.4'
sqlite_req = '>= 1.12.0'
libsoup_req = '>= 2.42'
introspection_req = '>= 0.6.3'
vala_req = '>= 0.11.0'
gtk_doc_req = '>= 1.9'

glib_dep = dependency('glib-2.0', version: glib_req)
gobject_dep = dependency('gobject-2.0', version: glib_req)
gio_dep = dependency('gio-2.0', version: glib_req)
clutter_dep = dependency('clutter-1.0', version: clutter_req)
cairo_dep = dependency('cairo', version: cairo_req)
sqlite_dep = dependency('sqlite3', version: sqlite_req)
libsoup_dep = dependency('libsoup-2.4', version: libsoup_req)
gtk_dep = dependency('gtk+-4.0')

introspection_dep = dependency('gobject-introspection-1.0', version: introspection_req, required: false)
vapigen_dep = dependency('vapigen', version: vala_req, required: false)
gtk_doc_dep = dependency('gtk-doc', version: gtk_doc_req, required: false)

# Configurations
config_h = configuration_data()
config_h.set_quoted('VERSION', version)

configure_file(
  output: 'config.h',
  configuration: config_h
)

# Options
build_demos = get_option('demos')

build_gtk_doc = get_option('gtk_doc')
if build_gtk_doc and not gtk_doc_dep.found()
  build_gtk_doc = false
endif

generate_gir = get_option('gir')
if generate_gir and not introspection_dep.found()
  generate_gir = false
endif

generate_vapi = generate_gir and get_option('vapi')
if generate_vapi and not vapigen_dep.found()
  generate_vapi = false
endif

subdir('shumate')

subdir('docs')

if build_demos == true
  subdir('demos')
endif

summary = [
  '',
  '------',
  'libshumate @0@ (@1@)'.format(version, api_version),
  '',
  '             Demos: @0@'.format(build_demos),
  '     Documentation: @0@'.format(build_gtk_doc),
  '     Introspection: @0@'.format(generate_gir),
  '          Vala API: @0@'.format(generate_vapi),
  '',
  'Directories:',
  '           prefix: @0@'.format(prefix),
  '       includedir: @0@'.format(includedir),
  '           libdir: @0@'.format(libdir),
  '          datadir: @0@'.format(datadir),
  '------',
]

message('\n'.join(summary))