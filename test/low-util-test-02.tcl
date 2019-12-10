source helpers.tcl
set design aes_cipher_top 

read_lef nangate45-bench/tech/NangateOpenCellLibrary.lef
read_def nangate45-bench/${design}/${design}_replace.def
legalize_placement
set def_file [make_result_file low-util-test-02-leg.def]
write_def $def_file
report_file $def_file
