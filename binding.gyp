{
  'targets': [
    {
      'target_name': 'addon',
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
        }, {
          'link_settings': {
            'libraries': ['-lbass'],
            'ldflags': [
              '-L<@(libs)/<(target_arch)/',
              '-L<@(libs)/<(target_arch)/plugins/',
              '-Wl,-rpath,<@(libs)/<(target_arch)'
            ]          
          }
        }]
      ]
    }
  ]
}