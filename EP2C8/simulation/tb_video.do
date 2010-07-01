vcom -93 -work work {C:/Projects/other/fpga/pet/simulation/tb_video.vhd}
vcom -93 -work work {C:/Projects/other/fpga/pet/simulation/tbc_sram.vhd}
vsim -t 1ps -L altera -L lpm -L sgate -L altera_mf -L cycloneii -L rtl_work -L work -voptargs="+acc" tb_video
vsim -L altera -L lpm -L sgate -L altera_mf -L cycloneii -L rtl_work -L work -voptargs=\"+acc\" -t 1ps tb_video 

add wave *

view structure
view signals
run 2 ms

wave zoomfull