project('ariel', 'c',
  version : '6.0.0',
  default_options : [
    'warning_level=3',
    'c_std=c11'
  ]
)

# Import filesystem module for directory checks
fs = import('fs')

# Host system detection
host_system = host_machine.system()
is_windows = host_system == 'windows'

# Dependencies
gtk4_dep = dependency('gtk4', version : '>= 4.0')

# lilv dependency - handle Windows differently due to cross-compilation path issues
if is_windows
  # For Windows cross-compilation, use reorganized win32 directory structure
  win32_inc_dir = meson.current_source_dir() / 'win32/include'
  win32_lib_dir = meson.current_source_dir() / 'win32/lib'
  
  lilv_dep = declare_dependency(
    include_directories: [
      include_directories('win32/include'),
      include_directories('win32/include/lilv-0'),
      include_directories('win32/include/serd-0'),
      include_directories('win32/include/sord-0'),
      include_directories('win32/include/sratom-0'),
      include_directories('win32/include/zix-0'),
      include_directories('win32/include/lv2')
    ],
    link_args: ['-L' + win32_lib_dir, '-llilv-0', '-lserd-0', '-lsord-0', '-lsratom-0', '-lzix-0']
  )
else
  lilv_dep = dependency('lilv-0', version : '>= 0.24')
endif

# Audio dependencies - different for Windows vs Linux
if is_windows
  # For Windows, use JACK from win32 directory structure
  win32_lib_dir = meson.current_source_dir() / 'win32/lib'
  win32_inc_dir = meson.current_source_dir() / 'win32/include'
  
  # Check if JACK libraries are available in win32 directory
  jack_lib_found = run_command('test', '-f', win32_lib_dir / 'libjack64.dll.a', check: false).returncode() == 0
  
  if jack_lib_found
    jack_dep = declare_dependency(
      include_directories: include_directories('win32/include/jack'),
      link_args: ['-L' + win32_lib_dir, '-ljack64']
    )
    message('Using JACK from win32 directory')
  else
    # Fallback for Windows builds without JACK
    jack_dep = declare_dependency(
      compile_args : ['-DJACK_OPTIONAL']
    )
    warning('JACK not found in win32 directory. Audio functionality will be limited.')
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

# Add Windows-specific WASAPI source
if is_windows
  sources += ['src/audio/wasapi_client.c']
endif

# Windows audio dependencies
if is_windows
  wasapi_dep = declare_dependency(
    link_args: ['-lole32', '-luuid', '-lwinmm', '-lksuser']
  )
  all_deps = [gtk4_dep, lilv_dep, jack_dep, wasapi_dep]
else
  all_deps = [gtk4_dep, lilv_dep, jack_dep]
endif

# Build executable
exe_name = is_windows ? 'ariel.exe' : 'ariel'
executable(exe_name,
  sources,
  dependencies : all_deps,
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

# CSS theme files and themes directory
install_data('data/ariel-theme.css',
  install_dir : get_option('datadir') / 'ariel'
)

# Install theme collection
if fs.is_dir('themes')
  install_subdir('themes',
    install_dir : get_option('datadir') / 'ariel',
    strip_directory : false
  )
endif

# Windows-specific: Install necessary DLLs and dependencies
if is_windows
  # Install DLLs from win32/bin if they exist
  win32_bin_dir = meson.current_source_dir() / 'win32/bin'
  if fs.is_dir(win32_bin_dir)
    message('Installing Windows DLLs from win32/bin')
    install_subdir('win32/bin',
      install_dir : get_option('bindir'),
      strip_directory : true
    )
  endif
  
  # Install any additional Windows-specific data
  win32_share_dir = meson.current_source_dir() / 'win32/share'
  if fs.is_dir(win32_share_dir)
    install_subdir('win32/share',
      install_dir : get_option('datadir'),
      strip_directory : true
    )
  endif
endif