# Nestastic
really bad 6502 emulator

# building:

- meson wrap install sdl2
- meson setup build
- meson compile -C build

# running:

`./build/nestastic`

# features:

- 30 instructions, some are almost certainly not correct.
- a probably pretty accurate cycle clocking system

enjoy! or don't, since it doesn't do really anything useful right now.

# building examples:

1) Install cc65
2) ca65 demo.s -o demo.o
3) ld65 demo.o -o demo.bin -C none.cfg
4) grab a dump of the output using `xxd -i demo.bin`
5) paste the output into src/emu/cpu/6502.cpp:229
6) watch it fail because most things aren't implemented yet.
