glib_prefix = glib_dep.get_pkgconfig_variable('prefix')
glib_docpath = join_paths(glib_prefix, 'share', 'gtk-doc', 'html', 'glib')

gobject_prefix = gobject_dep.get_pkgconfig_variable('prefix')
gobject_docpath = join_paths(gobject_prefix, 'share', 'gtk-doc', 'html', 'gobject')

gtk_prefix = gtk_dep.get_pkgconfig_variable('prefix')
gtk_docpath = join_paths(gtk_prefix, 'share', 'gtk-doc', 'html', 'gtk4')

libshumate_reference_ignored_h = [
  'shumate-enum-types.h',
  'shumate-marshal.h',
  'shumate.h',
]

libshumate_reference_source_folders = [
  join_paths(srcdir, 'shumate'),
  join_paths(builddir, 'shumate'),
]

version_xml = configuration_data()
version_xml.set('PACKAGE_VERSION', version)

configure_file(
  input: 'version.xml.in',
  output: 'version.xml',
  configuration: version_xml,
  install: false,
)

libshumate_reference_scan_args = [
  '--rebuild-types',
  '--deprecated-guards=GTK_DISABLE_DEPRECATED',
  '--ignore-headers=' + ' '.join(libshumate_reference_ignored_h),
]

libshumate_reference_mkdb_args = [
  '--sgml-mode',
  '--output-format=xml',
  '--name-space=shumate',
]

libshumate_reference_fixxref_args = [
  '--html-dir=@0@'.format(join_paths(gtkdocdir, 'html', package_string)),
  '--extra-dir=@0@'.format(glib_docpath),
  '--extra-dir=@0@'.format(gobject_docpath),
  '--extra-dir=@0@'.format(gtk_docpath),
]

gnome.gtkdoc(
  package_name,
  main_sgml: 'libshumate-docs.sgml',
  gobject_typesfile: files('libshumate.types'),
  src_dir: libshumate_reference_source_folders,
  dependencies: libshumate_dep,
  scan_args: libshumate_reference_scan_args,
  mkdb_args: libshumate_reference_mkdb_args,
  fixxref_args: libshumate_reference_fixxref_args,
  install: true,
  install_dir: join_paths(
    gtkdocdir,
    'html',
    package_string,
  )
)
