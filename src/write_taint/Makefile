all:
	g++ main.cpp taint_gen.cpp helper.cpp op_taint_gen.cpp VarWidth.cpp pass_info.cpp

debug:
	g++ -g main.cpp taint_gen.cpp helper.cpp op_taint_gen.cpp VarWidth.cpp pass_info.cpp

adder:
	./build/taint_gen ../../autoGenILA/naiveEg/word_adder/word_adder_yosys.v

naive:
	./build/taint_gen ../../autoGenILA/naiveEg/naive/naive_yosys.v

two:
	./build/taint_gen ../../autoGenILA/naiveEg/two_reg/two_reg_yosys.v

bound:
	./build/taint_gen ../../autoGenILA/naiveEg/bound_test/bound_test_yosys.v

case:
	./build/taint_gen ../../autoGenILA/naiveEg/case/case.v

128:
	./build/taint_gen ../../autoGenILA/AES/Verilog_wr/aes_128_yosys.v

aes:
	./build/taint_gen ../../autoGenILA/AES/Verilog_wr/aes_top_yosys.v

exp:
	./build/taint_gen ./experiment/experiment.v

pid:
	./build/taint_gen ../../autoGenILA/PID/SYN/PID_short.v

gb:
	./build/taint_gen ../../autoGenILA/GB/SYN_wr/gb_yosys.v

csimple:
	./build/taint_gen ../../autoGenILA/naiveEg/case_simple/case.v

rbm:
	./build/taint_gen ../../autoGenILA/RBM/SYN_wr/rbm_yosys.v

8051:
	./build/taint_gen ../../autoGenILA/8051/trunk/rtl/SYN_WR/oc8051_yosys.v

riscv:
	./build/taint_gen ../../autoGenILA/picorv32/SYN_wr/picorv32_yosys.v

piccolo:
	./build/taint_gen ../../autoGenILA/Piccolo-iu/verilog-cpu-old/SYN_wr/piccolo_yosys.v

sha_core:
	./build/taint_gen ../../autoGenILA/SHA/SYN_wr/sha_simp.v

sha:
	./build/taint_gen ../../autoGenILA/SHA/SYN_wr/sha_yosys.v

vpipe:
	./build/taint_gen ../../autoGenILA/vpipe/SYN/vpipe_yosys.v

dla:
	./build/taint_gen ../../autoGenILA/NVDLA/SYN_wr/nvdla_yosys.v

fsm:
	./build/taint_gen ../../autoGenILA/naiveEg/fsm/fsm_yosys.v

wcase:
	./build/taint_gen ../../autoGenILA/naiveEg/word_sum_case/word_adder_yosys.v

bi:
	./build/taint_gen ../../autoGenILA/biriscv/SYN_wr/biriscv_yosys.v

urv:
	./build/taint_gen ../../autoGenILA/ultra_riscv/SYN_wr/riscv_flatten_yosys.v

aeshls:
	./build/taint_gen ../../aes_hls/SYN/aes_yosys.v

eg:
	./build/taint_gen ./example/yosys_rtl/aes_yosys.v  
