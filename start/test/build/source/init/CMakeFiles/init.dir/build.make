# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/hsa/x86-code/start/test

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/hsa/x86-code/start/test/build

# Include any dependencies generated for this target.
include source/init/CMakeFiles/init.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include source/init/CMakeFiles/init.dir/compiler_depend.make

# Include the progress variables for this target.
include source/init/CMakeFiles/init.dir/progress.make

# Include the compile flags for this target's objects.
include source/init/CMakeFiles/init.dir/flags.make

source/init/CMakeFiles/init.dir/__/applib/crt0.S.o: source/init/CMakeFiles/init.dir/flags.make
source/init/CMakeFiles/init.dir/__/applib/crt0.S.o: ../source/applib/crt0.S
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/hsa/x86-code/start/test/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building ASM object source/init/CMakeFiles/init.dir/__/applib/crt0.S.o"
	cd /home/hsa/x86-code/start/test/build/source/init && /usr/bin/gcc $(ASM_DEFINES) $(ASM_INCLUDES) $(ASM_FLAGS) -o CMakeFiles/init.dir/__/applib/crt0.S.o -c /home/hsa/x86-code/start/test/source/applib/crt0.S

source/init/CMakeFiles/init.dir/__/applib/crt0.S.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing ASM source to CMakeFiles/init.dir/__/applib/crt0.S.i"
	cd /home/hsa/x86-code/start/test/build/source/init && /usr/bin/gcc $(ASM_DEFINES) $(ASM_INCLUDES) $(ASM_FLAGS) -E /home/hsa/x86-code/start/test/source/applib/crt0.S > CMakeFiles/init.dir/__/applib/crt0.S.i

source/init/CMakeFiles/init.dir/__/applib/crt0.S.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling ASM source to assembly CMakeFiles/init.dir/__/applib/crt0.S.s"
	cd /home/hsa/x86-code/start/test/build/source/init && /usr/bin/gcc $(ASM_DEFINES) $(ASM_INCLUDES) $(ASM_FLAGS) -S /home/hsa/x86-code/start/test/source/applib/crt0.S -o CMakeFiles/init.dir/__/applib/crt0.S.s

source/init/CMakeFiles/init.dir/__/applib/cstart.c.o: source/init/CMakeFiles/init.dir/flags.make
source/init/CMakeFiles/init.dir/__/applib/cstart.c.o: ../source/applib/cstart.c
source/init/CMakeFiles/init.dir/__/applib/cstart.c.o: source/init/CMakeFiles/init.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/hsa/x86-code/start/test/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object source/init/CMakeFiles/init.dir/__/applib/cstart.c.o"
	cd /home/hsa/x86-code/start/test/build/source/init && /usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT source/init/CMakeFiles/init.dir/__/applib/cstart.c.o -MF CMakeFiles/init.dir/__/applib/cstart.c.o.d -o CMakeFiles/init.dir/__/applib/cstart.c.o -c /home/hsa/x86-code/start/test/source/applib/cstart.c

source/init/CMakeFiles/init.dir/__/applib/cstart.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/init.dir/__/applib/cstart.c.i"
	cd /home/hsa/x86-code/start/test/build/source/init && /usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/hsa/x86-code/start/test/source/applib/cstart.c > CMakeFiles/init.dir/__/applib/cstart.c.i

source/init/CMakeFiles/init.dir/__/applib/cstart.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/init.dir/__/applib/cstart.c.s"
	cd /home/hsa/x86-code/start/test/build/source/init && /usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/hsa/x86-code/start/test/source/applib/cstart.c -o CMakeFiles/init.dir/__/applib/cstart.c.s

source/init/CMakeFiles/init.dir/__/applib/lib_syscall.c.o: source/init/CMakeFiles/init.dir/flags.make
source/init/CMakeFiles/init.dir/__/applib/lib_syscall.c.o: ../source/applib/lib_syscall.c
source/init/CMakeFiles/init.dir/__/applib/lib_syscall.c.o: source/init/CMakeFiles/init.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/hsa/x86-code/start/test/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object source/init/CMakeFiles/init.dir/__/applib/lib_syscall.c.o"
	cd /home/hsa/x86-code/start/test/build/source/init && /usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT source/init/CMakeFiles/init.dir/__/applib/lib_syscall.c.o -MF CMakeFiles/init.dir/__/applib/lib_syscall.c.o.d -o CMakeFiles/init.dir/__/applib/lib_syscall.c.o -c /home/hsa/x86-code/start/test/source/applib/lib_syscall.c

source/init/CMakeFiles/init.dir/__/applib/lib_syscall.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/init.dir/__/applib/lib_syscall.c.i"
	cd /home/hsa/x86-code/start/test/build/source/init && /usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/hsa/x86-code/start/test/source/applib/lib_syscall.c > CMakeFiles/init.dir/__/applib/lib_syscall.c.i

source/init/CMakeFiles/init.dir/__/applib/lib_syscall.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/init.dir/__/applib/lib_syscall.c.s"
	cd /home/hsa/x86-code/start/test/build/source/init && /usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/hsa/x86-code/start/test/source/applib/lib_syscall.c -o CMakeFiles/init.dir/__/applib/lib_syscall.c.s

source/init/CMakeFiles/init.dir/main.c.o: source/init/CMakeFiles/init.dir/flags.make
source/init/CMakeFiles/init.dir/main.c.o: ../source/init/main.c
source/init/CMakeFiles/init.dir/main.c.o: source/init/CMakeFiles/init.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/hsa/x86-code/start/test/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object source/init/CMakeFiles/init.dir/main.c.o"
	cd /home/hsa/x86-code/start/test/build/source/init && /usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT source/init/CMakeFiles/init.dir/main.c.o -MF CMakeFiles/init.dir/main.c.o.d -o CMakeFiles/init.dir/main.c.o -c /home/hsa/x86-code/start/test/source/init/main.c

source/init/CMakeFiles/init.dir/main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/init.dir/main.c.i"
	cd /home/hsa/x86-code/start/test/build/source/init && /usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/hsa/x86-code/start/test/source/init/main.c > CMakeFiles/init.dir/main.c.i

source/init/CMakeFiles/init.dir/main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/init.dir/main.c.s"
	cd /home/hsa/x86-code/start/test/build/source/init && /usr/bin/gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/hsa/x86-code/start/test/source/init/main.c -o CMakeFiles/init.dir/main.c.s

# Object files for target init
init_OBJECTS = \
"CMakeFiles/init.dir/__/applib/crt0.S.o" \
"CMakeFiles/init.dir/__/applib/cstart.c.o" \
"CMakeFiles/init.dir/__/applib/lib_syscall.c.o" \
"CMakeFiles/init.dir/main.c.o"

# External object files for target init
init_EXTERNAL_OBJECTS =

source/init/init: source/init/CMakeFiles/init.dir/__/applib/crt0.S.o
source/init/init: source/init/CMakeFiles/init.dir/__/applib/cstart.c.o
source/init/init: source/init/CMakeFiles/init.dir/__/applib/lib_syscall.c.o
source/init/init: source/init/CMakeFiles/init.dir/main.c.o
source/init/init: source/init/CMakeFiles/init.dir/build.make
source/init/init: source/init/CMakeFiles/init.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/hsa/x86-code/start/test/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Linking C executable init"
	cd /home/hsa/x86-code/start/test/build/source/init && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/init.dir/link.txt --verbose=$(VERBOSE)
	cd /home/hsa/x86-code/start/test/build/source/init && objcopy -S init.elf /home/hsa/x86-code/start/test/../../image/init.elf
	cd /home/hsa/x86-code/start/test/build/source/init && objdump -x -d -S -m i386 /home/hsa/x86-code/start/test/build/source/init/init.elf > init_dis.txt
	cd /home/hsa/x86-code/start/test/build/source/init && readelf -a /home/hsa/x86-code/start/test/build/source/init/init.elf > init_elf.txt

# Rule to build all files generated by this target.
source/init/CMakeFiles/init.dir/build: source/init/init
.PHONY : source/init/CMakeFiles/init.dir/build

source/init/CMakeFiles/init.dir/clean:
	cd /home/hsa/x86-code/start/test/build/source/init && $(CMAKE_COMMAND) -P CMakeFiles/init.dir/cmake_clean.cmake
.PHONY : source/init/CMakeFiles/init.dir/clean

source/init/CMakeFiles/init.dir/depend:
	cd /home/hsa/x86-code/start/test/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/hsa/x86-code/start/test /home/hsa/x86-code/start/test/source/init /home/hsa/x86-code/start/test/build /home/hsa/x86-code/start/test/build/source/init /home/hsa/x86-code/start/test/build/source/init/CMakeFiles/init.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : source/init/CMakeFiles/init.dir/depend

