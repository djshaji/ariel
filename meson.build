project('ariel', 'c',
  version : '6.0.0',
  default_options : [
    'warning_level=3',
    'c_std=c11'
  ]
)

# Host system detection
host_system = host_machine.system()
is_windows = host_system == 'windows'

# Dependencies
gtk4_dep = dependency('gtk4', version : '>= 4.0')
lilv_dep = dependency('lilv-0', version : '>= 0.24')

# Audio dependencies - different for Windows vs Linux
if is_windows
  # For Windows, we'll use JACK for Windows or ASIO
  jack_dep = dependency('jack', version : '>= 1.0', required : false)
  if not jack_dep.found()
    # Fallback to finding JACK manually on Windows
    jack_dep = declare_dependency(
      link_args : ['-ljack64'],
      compile_args : ['-DHAVE_JACK']
    )
  endif
else
  # Linux/Unix systems
  jack_dep = dependency('jack', version : '>= 1.0', required : false)
  
  # If JACK is not found, try PipeWire-JACK
  if not jack_dep.found()
    jack_dep = dependency('jack', version : '>= 1.0', required : true,
                         fallback : ['pipewire', 'pipewire_jack_dep'])
  endif
endif

# Include directories
inc = include_directories('include')

# Source files
sources = [
  'src/main.c',
  'src/ariel_log.c',
  'src/ui/window.c',
  'src/ui/plugin_list.c',
  'src/ui/mixer.c',
  'src/ui/transport.c',
  'src/ui/settings.c',
  'src/ui/parameter_controls.c',
  'src/ui/active_plugins.c',
  'src/audio/engine.c',
  'src/audio/plugin_manager.c',
  'src/audio/jack_client.c',
  'src/audio/config.c',
  'src/audio/active_plugin.c'  
]

# Build executable
exe_name = is_windows ? 'ariel.exe' : 'ariel'
executable(exe_name,
  sources,
  dependencies : [gtk4_dep, lilv_dep, jack_dep],
  include_directories : inc,
  install : true,
  win_subsystem : is_windows ? 'windows' : 'console'
)

# Desktop file and installation (Linux only)
if not is_windows
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
endif

# CSS theme file
install_data('data/ariel-theme.css',
  install_dir : is_windows ? get_option('datadir') / 'ariel' : get_option('datadir') / 'ariel'
)