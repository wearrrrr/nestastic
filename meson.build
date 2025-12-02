project('nestastic', 'cpp', default_options: 'default_library=static')

dep_sdl2 = dependency('sdl2')

executable(
  'nestastic',
  ['src/main.cpp', 'src/emu/cpu/6502.cpp', 'src/emu/cpu/6502.h'],
  win_subsystem: 'windows',
  dependencies: [dep_sdl2]
)
