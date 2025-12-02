.org $8000

start:
        LDX #$0A
loop:
        DEX
        BNE loop

done:
        JMP done

        .org $FFFC
        .word start
