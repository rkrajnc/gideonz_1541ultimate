    .export     _sid_player
    .export     _sid_player_end
    
.segment    "RODATA"
_sid_player:
    .incbin "../../../data/src/sid_play.bin"
_sid_player_end:
    .byte 0
