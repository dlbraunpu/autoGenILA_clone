# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.14

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /opt/cmake-3.14.4-Linux-x86_64/bin/cmake

# The command to remove a file.
RM = /opt/cmake-3.14.4-Linux-x86_64/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /workspace/research/ILA/autoGenILA/src/taint_method

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /workspace/research/ILA/autoGenILA/src/taint_method

# Include any dependencies generated for this target.
include CMakeFiles/TaintGenLib.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/TaintGenLib.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/TaintGenLib.dir/flags.make

CMakeFiles/TaintGenLib.dir/VarWidth.cpp.o: CMakeFiles/TaintGenLib.dir/flags.make
CMakeFiles/TaintGenLib.dir/VarWidth.cpp.o: VarWidth.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/workspace/research/ILA/autoGenILA/src/taint_method/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/TaintGenLib.dir/VarWidth.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/TaintGenLib.dir/VarWidth.cpp.o -c /workspace/research/ILA/autoGenILA/src/taint_method/VarWidth.cpp

CMakeFiles/TaintGenLib.dir/VarWidth.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/TaintGenLib.dir/VarWidth.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /workspace/research/ILA/autoGenILA/src/taint_method/VarWidth.cpp > CMakeFiles/TaintGenLib.dir/VarWidth.cpp.i

CMakeFiles/TaintGenLib.dir/VarWidth.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/TaintGenLib.dir/VarWidth.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /workspace/research/ILA/autoGenILA/src/taint_method/VarWidth.cpp -o CMakeFiles/TaintGenLib.dir/VarWidth.cpp.s

CMakeFiles/TaintGenLib.dir/helper.cpp.o: CMakeFiles/TaintGenLib.dir/flags.make
CMakeFiles/TaintGenLib.dir/helper.cpp.o: helper.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/workspace/research/ILA/autoGenILA/src/taint_method/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/TaintGenLib.dir/helper.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/TaintGenLib.dir/helper.cpp.o -c /workspace/research/ILA/autoGenILA/src/taint_method/helper.cpp

CMakeFiles/TaintGenLib.dir/helper.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/TaintGenLib.dir/helper.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /workspace/research/ILA/autoGenILA/src/taint_method/helper.cpp > CMakeFiles/TaintGenLib.dir/helper.cpp.i

CMakeFiles/TaintGenLib.dir/helper.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/TaintGenLib.dir/helper.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /workspace/research/ILA/autoGenILA/src/taint_method/helper.cpp -o CMakeFiles/TaintGenLib.dir/helper.cpp.s

CMakeFiles/TaintGenLib.dir/main.cpp.o: CMakeFiles/TaintGenLib.dir/flags.make
CMakeFiles/TaintGenLib.dir/main.cpp.o: main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/workspace/research/ILA/autoGenILA/src/taint_method/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/TaintGenLib.dir/main.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/TaintGenLib.dir/main.cpp.o -c /workspace/research/ILA/autoGenILA/src/taint_method/main.cpp

CMakeFiles/TaintGenLib.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/TaintGenLib.dir/main.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /workspace/research/ILA/autoGenILA/src/taint_method/main.cpp > CMakeFiles/TaintGenLib.dir/main.cpp.i

CMakeFiles/TaintGenLib.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/TaintGenLib.dir/main.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /workspace/research/ILA/autoGenILA/src/taint_method/main.cpp -o CMakeFiles/TaintGenLib.dir/main.cpp.s

CMakeFiles/TaintGenLib.dir/op_taint_gen.cpp.o: CMakeFiles/TaintGenLib.dir/flags.make
CMakeFiles/TaintGenLib.dir/op_taint_gen.cpp.o: op_taint_gen.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/workspace/research/ILA/autoGenILA/src/taint_method/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object CMakeFiles/TaintGenLib.dir/op_taint_gen.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/TaintGenLib.dir/op_taint_gen.cpp.o -c /workspace/research/ILA/autoGenILA/src/taint_method/op_taint_gen.cpp

CMakeFiles/TaintGenLib.dir/op_taint_gen.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/TaintGenLib.dir/op_taint_gen.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /workspace/research/ILA/autoGenILA/src/taint_method/op_taint_gen.cpp > CMakeFiles/TaintGenLib.dir/op_taint_gen.cpp.i

CMakeFiles/TaintGenLib.dir/op_taint_gen.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/TaintGenLib.dir/op_taint_gen.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /workspace/research/ILA/autoGenILA/src/taint_method/op_taint_gen.cpp -o CMakeFiles/TaintGenLib.dir/op_taint_gen.cpp.s

CMakeFiles/TaintGenLib.dir/taint_gen.cpp.o: CMakeFiles/TaintGenLib.dir/flags.make
CMakeFiles/TaintGenLib.dir/taint_gen.cpp.o: taint_gen.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/workspace/research/ILA/autoGenILA/src/taint_method/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object CMakeFiles/TaintGenLib.dir/taint_gen.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/TaintGenLib.dir/taint_gen.cpp.o -c /workspace/research/ILA/autoGenILA/src/taint_method/taint_gen.cpp

CMakeFiles/TaintGenLib.dir/taint_gen.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/TaintGenLib.dir/taint_gen.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /workspace/research/ILA/autoGenILA/src/taint_method/taint_gen.cpp > CMakeFiles/TaintGenLib.dir/taint_gen.cpp.i

CMakeFiles/TaintGenLib.dir/taint_gen.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/TaintGenLib.dir/taint_gen.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /workspace/research/ILA/autoGenILA/src/taint_method/taint_gen.cpp -o CMakeFiles/TaintGenLib.dir/taint_gen.cpp.s

# Object files for target TaintGenLib
TaintGenLib_OBJECTS = \
"CMakeFiles/TaintGenLib.dir/VarWidth.cpp.o" \
"CMakeFiles/TaintGenLib.dir/helper.cpp.o" \
"CMakeFiles/TaintGenLib.dir/main.cpp.o" \
"CMakeFiles/TaintGenLib.dir/op_taint_gen.cpp.o" \
"CMakeFiles/TaintGenLib.dir/taint_gen.cpp.o"

# External object files for target TaintGenLib
TaintGenLib_EXTERNAL_OBJECTS =

libTaintGenLib.a: CMakeFiles/TaintGenLib.dir/VarWidth.cpp.o
libTaintGenLib.a: CMakeFiles/TaintGenLib.dir/helper.cpp.o
libTaintGenLib.a: CMakeFiles/TaintGenLib.dir/main.cpp.o
libTaintGenLib.a: CMakeFiles/TaintGenLib.dir/op_taint_gen.cpp.o
libTaintGenLib.a: CMakeFiles/TaintGenLib.dir/taint_gen.cpp.o
libTaintGenLib.a: CMakeFiles/TaintGenLib.dir/build.make
libTaintGenLib.a: CMakeFiles/TaintGenLib.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/workspace/research/ILA/autoGenILA/src/taint_method/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Linking CXX static library libTaintGenLib.a"
	$(CMAKE_COMMAND) -P CMakeFiles/TaintGenLib.dir/cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/TaintGenLib.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/TaintGenLib.dir/build: libTaintGenLib.a

.PHONY : CMakeFiles/TaintGenLib.dir/build

CMakeFiles/TaintGenLib.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/TaintGenLib.dir/cmake_clean.cmake
.PHONY : CMakeFiles/TaintGenLib.dir/clean

CMakeFiles/TaintGenLib.dir/depend:
	cd /workspace/research/ILA/autoGenILA/src/taint_method && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /workspace/research/ILA/autoGenILA/src/taint_method /workspace/research/ILA/autoGenILA/src/taint_method /workspace/research/ILA/autoGenILA/src/taint_method /workspace/research/ILA/autoGenILA/src/taint_method /workspace/research/ILA/autoGenILA/src/taint_method/CMakeFiles/TaintGenLib.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/TaintGenLib.dir/depend

