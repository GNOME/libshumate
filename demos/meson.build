libshumate_demos = [
  ['minimal', 'minimal.c'],
  ['launcher', ['launcher.c', 'markers.c']],
  ['url-marker', 'url-marker.c'],
  ['create_destroy_test', 'create-destroy-test.c'],
]

foreach demo: libshumate_demos
  demo_name = demo.get(0)
  demo_sources = demo.get(1)

  executable(
    demo_name,
    demo_sources,
    install: true,
    dependencies: [
      libshumate_dep,
    ]
  )
endforeach

subdir('icons')

if generate_vapi == true
  libshumate_vala_demos = [
    ['launcher-vala', ['launcher-vala.vala', 'markers-vala.vala']],
  ]

  add_languages('vala')

  foreach demo: libshumate_vala_demos
    demo_name = demo.get(0)
    demo_sources = demo.get(1)

    executable(
      demo_name,
      demo_sources,
      install: false,
      dependencies: [
        libshumate_dep,
        libshumate_vapi,
      ]
    )
  endforeach
endif
