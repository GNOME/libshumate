if get_option('gtk_doc')

expand_content_md_files = [
]

libshumate_doc_toml = configure_file(
  input: 'libshumate.toml.in',
  output: 'libshumate.toml',
  configuration: {
    'VERSION': meson.project_version(),
  }
)

custom_target('libshumate-doc',
  input: [ libshumate_doc_toml, libshumate_gir[0] ],
  output: 'libshumate-@0@'.format(lib_version),
  command: [
    gidocgen,
    'generate',
    '--quiet',
    '--fatal-warnings',
    '--add-include-path=@0@'.format(meson.current_build_dir() / '../shumate'),
    '--config=@INPUT0@',
    '--output-dir=@OUTPUT@',
    '--no-namespace-dir',
    '--content-dir=@0@'.format(meson.current_source_dir()),
    '@INPUT1@',
  ],
  depend_files: [ expand_content_md_files ],
  build_by_default: true,
  install: true,
  install_dir: docs_dir,
)

endif
