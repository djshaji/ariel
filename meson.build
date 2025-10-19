project('ariel', 'c',
  version : '0.1.0',
  default_options : [
    'warning_level=3',
    'c_std=c11'
  ]
)

# Dependencies
gtk4_dep = dependency('gtk4', version : '>= 4.0')
lilv_dep = dependency('lilv-0', version : '>= 0.24')
jack_dep = dependency('jack', version : '>= 1.0', required : false)

# If JACK is not found, try PipeWire-JACK
if not jack_dep.found()
  jack_dep = dependency('jack', version : '>= 1.0', required : true,
                       fallback : ['pipewire', 'pipewire_jack_dep'])
endif

# Include directories
inc = include_directories('include')

# Source files
sources = [
  'src/main.c',
  'src/ui/window.c',
  'src/ui/plugin_list.c',
  'src/ui/mixer.c',
  'src/ui/transport.c',
  'src/ui/parameter_controls.c',
  'src/ui/active_plugins.c',
  'src/audio/engine.c',
  'src/audio/plugin_manager.c',
  'src/audio/jack_client.c',
  'src/audio/config.c',
  'src/audio/active_plugin.c'  
]

# Build executable
executable('ariel',
  sources,
  dependencies : [gtk4_dep, lilv_dep, jack_dep],
  include_directories : inc,
  install : true
)

# Desktop file
desktop_file = configure_file(
  input : 'data/ariel.desktop.in',
  output : 'ariel.desktop',
  configuration : {'bindir' : get_option('prefix') / get_option('bindir')}
)

install_data(desktop_file,
  install_dir : get_option('datadir') / 'applications'
)

# AppStream metadata
install_data('data/ariel.appdata.xml',
  install_dir : get_option('datadir') / 'metainfo'
)