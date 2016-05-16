{
  'targets': [
  {
    'target_name': 'player',
    'sources': [
      'addon.cc',
      'player.cc'
    ],
    'variables': {
      'libs': '<(module_root_dir)/<(OS)'
    },
    'conditions': [
      ['OS == "mac"', {
        'include_dirs': ['<(libs)'],
        'link_settings': {
          'libraries': ['-lbass'],
          'ldflags': [
            '-L<@(libs)',
            '-Wl,-rpath,<@(libs)'
          ]          
        }
      }],
      ['OS == "win"', {
        'link_settings': {
          'libraries': [
            '-l<@(libs)/<(target_arch)/bass',
            '-l<@(libs)/<(target_arch)/plugins/bassmidi',
            '-l<@(libs)/<(target_arch)/plugins/bassflac'],
          'ldflags': [
            '-Wl,-rpath,<@(libs)/<(target_arch)'
          ]          
        }
      }],
      ['OS == "linux"', {
        'link_settings': {
          'libraries': ['-lbass','-lbassmidi','-lbassflac'],
          'ldflags': [
            '-L<@(libs)/<(target_arch)/',
            '-L<@(libs)/<(target_arch)/plugins/',
            '-Wl,-rpath,<@(libs)/<(target_arch)'
          ]          
        }
      }]
    ]}, {
	  'target_name': 'copy_binary',
    'type':'none',
    'dependencies' : ['player'],
    'conditions': [
      ['OS == "mac"', {
        'copies': [{
          'destination': '<(module_root_dir)/build/Release/',
          'files': ['<(module_root_dir)/<(OS)/libbass.dylib']
        }]
      }],
      ['OS == "win"', {
        'copies': [{
          'destination': '<(module_root_dir)/build/Release/',
          'files': ['<(module_root_dir)/<(OS)/<(target_arch)/bass.dll']
        }]
      }],
      ['OS == "linux"', {
        'copies': [{
          'destination': '<(module_root_dir)/build/Release/',
          'files': ['<(module_root_dir)/<(OS)/<(target_arch)/libbass.so']
        }]
      }]
    ]
  }]
}