.org $8000

start:
        LDA #$10        ; load A
        ADC #$05        ; add 5 → A = 0x15

done:
        JMP start

.org $FFFC
        .word start
