libshumate_sources += files(
  'shumate-vector-background-layer.c',
  'shumate-vector-collision.c',
  'shumate-vector-expression.c',
  'shumate-vector-expression-interpolate.c',
  'shumate-vector-expression-filter.c',
  'shumate-vector-fill-layer.c',
  'shumate-vector-index.c',
  'shumate-vector-layer.c',
  'shumate-vector-line-layer.c',
  'shumate-vector-render-scope.c',
  'shumate-vector-symbol.c',
  'shumate-vector-symbol-container.c',
  'shumate-vector-symbol-info.c',
  'shumate-vector-symbol-layer.c',
  'shumate-vector-utils.c',
  'shumate-vector-value.c',
  'vector_tile.pb-c.c',
)

libshumate_deps += [
  dependency('json-glib-1.0', version: json_glib_req),
  dependency('libprotobuf-c'),
]

libshumate_c_args += [
  '-DSHUMATE_HAS_VECTOR_RENDERER',
]

gperf = find_program('gperf')
libshumate_expr_gperf = custom_target (
  'shumate-vector-expression-gperf',
  output: 'shumate-vector-expression-gperf.c',
  input: 'shumate-vector-expression-type.gperf',
  command: [
    gperf,
    '--readonly-tables',
    '--includes',
    '--struct-type',
    '--omit-struct-type',
    '--lookup-function-name=shumate_vector_expression_type_lookup',
    '--hash-function-name=shumate_vector_expression_type_hash',
    '--output-file=@OUTPUT@',
    '@INPUT@'
  ],
)

libshumate_sources += [libshumate_expr_gperf]