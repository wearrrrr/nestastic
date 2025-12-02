project('nestastic', 'cpp', default_options: 'default_library=static')

dep_sdl2 = dependency('sdl2')

executable(
  'nestastic',
  ['src/main.cpp', 'src/emu/bus/bus.cpp', 'src/emu/cartridge/cartridge.cpp', 'src/emu/CPU/6502.cpp', 'src/emu/PPU/ppu.cpp'],
  win_subsystem: 'windows',
  dependencies: [dep_sdl2]
)
