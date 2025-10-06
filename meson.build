project(
  'libshumate', 'c',
  version: '1.6.alpha',
  license: 'LGPLv2.1+',
  meson_version: '>= 0.57.0',
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

api_version = '1.0'
lib_version = '1.0'

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
docs_dir = datadir / 'doc'
vapidir = join_paths(datadir, 'vala', 'vapi')

# Dependencies
cc = meson.get_compiler('c')

libm_dep = cc.find_library('m', required: true)

glib_req = '>= 2.74.0'
cairo_req = '>= 1.4'
sqlite_req = '>= 1.12.0'
introspection_req = '>= 0.6.3'
vala_req = '>= 0.11.0'
json_glib_req = '>= 1.6'

glib_dep = dependency('glib-2.0', version: glib_req)
gobject_dep = dependency('gobject-2.0', version: glib_req)
gio_dep = dependency('gio-2.0', version: glib_req)
cairo_dep = dependency('cairo', version: cairo_req)
sqlite_dep = dependency('sqlite3', version: sqlite_req)
gtk_dep = dependency('gtk4')
libsoup_dep = dependency('libsoup-3.0', version: '>= 3.0')

gi_scanner_dep = find_program('g-ir-scanner', required: false)
introspection_dep = dependency('gobject-introspection-1.0', version: introspection_req, required: false)
vapigen_dep = find_program('vapigen', version: vala_req, native: true, required: false)

libsysprof_capture_dep = dependency('sysprof-capture-4',
  required: get_option('sysprof'),
  default_options: [
    'enable_examples=false',
    'enable_gtk=false',
    'enable_tests=false',
    'enable_tools=false',
    'libsysprof=false',
    'with_sysprofd=none',
    'help=false',
  ],
  fallback: ['sysprof', 'libsysprof_capture_dep'],
)
if libsysprof_capture_dep.found()
  add_project_arguments('-DSHUMATE_HAS_SYSPROF=1', language: 'c')
endif

if get_option('gtk_doc')
  dependency('gi-docgen',
    version: '>= 2021.1',
    fallback: ['gi-docgen', 'dummy_dep'],
    native : true,
  )
  gidocgen = find_program('gi-docgen')
endif

add_project_arguments('-DSHUMATE_VERSION="@0@"'.format(meson.project_version()), language: 'c')
add_project_arguments('-DGETTEXT_PACKAGE="shumate1"', language: 'c')

# Options
build_demos = get_option('demos')

if get_option('gtk_doc') and not get_option('gir')
  error('Cannot generate the documentation if gobject-introspection is disabled.')
endif

generate_gir = get_option('gir')
if generate_gir and not introspection_dep.found()
  generate_gir = false
endif

generate_vapi = generate_gir and get_option('vapi')
if generate_vapi and not vapigen_dep.found()
  generate_vapi = false
endif

subdir('data')

subdir('shumate')

subdir('po')

subdir('docs')

if build_demos == true
  subdir('demos')
endif

subdir('tests')

summary = [
  '',
  '------',
  'libshumate @0@ (@1@)'.format(version, api_version),
  '',
  '             Demos: @0@'.format(build_demos),
  '     Documentation: @0@'.format(get_option('gtk_doc')),
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
