# Nestastic
rather decent NES emulator

# building:

- ./install_deps.sh
- meson setup build
- meson compile -C build

# running:

`./build/nestastic`

# features:

- Mapper 0 and 2 support
- (WIP) ImGui-based GUI for debugging and memory viewing.

enjoy!

# building examples:

1) Install cc65
2) ca65 demo.s -o demo.o
3) ld65 demo.o -o demo.bin -C none.cfg
4) grab a dump of the output using `xxd -i demo.bin`
5) paste the output into src/emu/cpu/6502.cpp:229
6) watch it fail because most things aren't implemented yet.
