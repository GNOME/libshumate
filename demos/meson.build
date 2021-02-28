shumate_demo_resources = gnome.compile_resources(
  'shumate-demo-resources',
  'org.gnome.Shumate.Demo.gresources.xml',
  c_name: 'shumate_demo',
)

shumate_demo_sources = [
  shumate_demo_resources,

  'shumate-demo.c',
  'shumate-demo-window.c',
  'shumate-test-tile-source.c',
]

shumate_demo = executable(
  'shumate-demo',
  shumate_demo_sources,
  install: true,
  dependencies: [
    libshumate_dep,
  ]
)
