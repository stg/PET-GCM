DEFINE FPGA, SPACE =  RAM, ORG = %800000
SEGMENT FPGA

XDEF _video_ram
XDEF _input
XDEF _dma
XDEF _audio_ram

ORG %800000
_video_ram  DS %40000
ORG %840000
_input      DS %20000
ORG %860000
_dma        DS %20000
ORG %880000
_audio_ram DS %80000