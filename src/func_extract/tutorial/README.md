# Tutorial for generating update functions
This is a tutorial for how to generate update functions from RTL designs. We use PicoRISCV as the example:
1. "RTL" folder contains the RTL code for PicoRISCV.
2. "gen_verilog_script.ys" reads the RTL, and generate a simplied new RTL with Yosys. The generated file, design.v, is put in the "files" folder.
3. "gen_rst_analysis.ys" reads the RTL, and generate the reset values for each signal in the design. The generated "rst.vcd" is also put in the "files" folder. The reset values are generated with Yosys. You can also use other approaches, e.g., run RTL simulations with reset inputs, using any simulation tools.
4. The "files" folder contains all the files that we need to prepare for the tool to run:
- allowed_target.txt: (I know this may not be a good name, bear with me): it specifies the target register that you want to generate update functions for. If it is a single register, the format is: *name*: *cycle*, where *name* is the register name, and *cycle* is the upper bound of the exection cycles of this instruction. You can find out more about why the upper bound is used in the paper.
- config.txt: this contains some configurations for the update function generation program. This is only for experimental purpose, usually you do not need to modify it.
- instr.txt: this contains information for the instruction we are interested in. See the comments in the files for details.
5. How to run: in the func_extract directory, the first time you need to type "make tutorial_clean". Then you will see the generated LLVM-IR based update functions in "files" folder. tutorial_clean does some file cleaning, which only needs to be done once. Later, you only need to type "make tutorial" to generate the update functions.
6. In the generated files you can find these files:
- 'add_cpuregs[1]_Arr_8_tmp.ll': a temporal file, just ignore it.
- 'add_cpuregs[1]_Arr_8.ll': the original llvm-based update function for the array of cpuregs by the add instruction, without being optimized by LLVM
- 'add_cpuregs[1]_Arr_8_clean.ll': a cleaner version by removing most comments
- 'add_cpuregs[1]_Arr_8_clean.o3.ll': the final update function we are looking for! It has been optimized by LLVM-o3 optimization.
