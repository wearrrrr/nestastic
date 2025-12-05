project('nestastic', 'cpp', default_options: 'default_library=static')

dep_sdl2 = dependency('sdl2')
dep_imgui = dependency('imgui-docking')

executable(
  'nestastic',
  ['src/main.cpp', 'src/emu/bus/bus.cpp', 'src/emu/cartridge/cartridge.cpp', 'src/emu/CPU/CPU.cpp', 'src/emu/PPU/ppu.cpp', 'src/emu/mapper/mapper.cpp', 'src/emu/mapper/000/000.cpp', 'src/emu/mapper/002/002.cpp'],
  win_subsystem: 'windows',
  dependencies: [dep_sdl2, dep_imgui]
)
