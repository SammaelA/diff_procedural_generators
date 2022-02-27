# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/sammael/grade/dependencies/Hydra/HydraAPI

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/sammael/grade/dependencies/Hydra/HydraAPI

# Include any dependencies generated for this target.
include utils/mikktspace/CMakeFiles/mikktspace.dir/depend.make

# Include the progress variables for this target.
include utils/mikktspace/CMakeFiles/mikktspace.dir/progress.make

# Include the compile flags for this target's objects.
include utils/mikktspace/CMakeFiles/mikktspace.dir/flags.make

utils/mikktspace/CMakeFiles/mikktspace.dir/mikktspace.c.o: utils/mikktspace/CMakeFiles/mikktspace.dir/flags.make
utils/mikktspace/CMakeFiles/mikktspace.dir/mikktspace.c.o: utils/mikktspace/mikktspace.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/sammael/grade/dependencies/Hydra/HydraAPI/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object utils/mikktspace/CMakeFiles/mikktspace.dir/mikktspace.c.o"
	cd /home/sammael/grade/dependencies/Hydra/HydraAPI/utils/mikktspace && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/mikktspace.dir/mikktspace.c.o   -c /home/sammael/grade/dependencies/Hydra/HydraAPI/utils/mikktspace/mikktspace.c

utils/mikktspace/CMakeFiles/mikktspace.dir/mikktspace.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/mikktspace.dir/mikktspace.c.i"
	cd /home/sammael/grade/dependencies/Hydra/HydraAPI/utils/mikktspace && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/sammael/grade/dependencies/Hydra/HydraAPI/utils/mikktspace/mikktspace.c > CMakeFiles/mikktspace.dir/mikktspace.c.i

utils/mikktspace/CMakeFiles/mikktspace.dir/mikktspace.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/mikktspace.dir/mikktspace.c.s"
	cd /home/sammael/grade/dependencies/Hydra/HydraAPI/utils/mikktspace && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/sammael/grade/dependencies/Hydra/HydraAPI/utils/mikktspace/mikktspace.c -o CMakeFiles/mikktspace.dir/mikktspace.c.s

utils/mikktspace/CMakeFiles/mikktspace.dir/mikktspace.c.o.requires:

.PHONY : utils/mikktspace/CMakeFiles/mikktspace.dir/mikktspace.c.o.requires

utils/mikktspace/CMakeFiles/mikktspace.dir/mikktspace.c.o.provides: utils/mikktspace/CMakeFiles/mikktspace.dir/mikktspace.c.o.requires
	$(MAKE) -f utils/mikktspace/CMakeFiles/mikktspace.dir/build.make utils/mikktspace/CMakeFiles/mikktspace.dir/mikktspace.c.o.provides.build
.PHONY : utils/mikktspace/CMakeFiles/mikktspace.dir/mikktspace.c.o.provides

utils/mikktspace/CMakeFiles/mikktspace.dir/mikktspace.c.o.provides.build: utils/mikktspace/CMakeFiles/mikktspace.dir/mikktspace.c.o


# Object files for target mikktspace
mikktspace_OBJECTS = \
"CMakeFiles/mikktspace.dir/mikktspace.c.o"

# External object files for target mikktspace
mikktspace_EXTERNAL_OBJECTS =

bin/libmikktspace.a: utils/mikktspace/CMakeFiles/mikktspace.dir/mikktspace.c.o
bin/libmikktspace.a: utils/mikktspace/CMakeFiles/mikktspace.dir/build.make
bin/libmikktspace.a: utils/mikktspace/CMakeFiles/mikktspace.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/sammael/grade/dependencies/Hydra/HydraAPI/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C static library ../../bin/libmikktspace.a"
	cd /home/sammael/grade/dependencies/Hydra/HydraAPI/utils/mikktspace && $(CMAKE_COMMAND) -P CMakeFiles/mikktspace.dir/cmake_clean_target.cmake
	cd /home/sammael/grade/dependencies/Hydra/HydraAPI/utils/mikktspace && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/mikktspace.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
utils/mikktspace/CMakeFiles/mikktspace.dir/build: bin/libmikktspace.a

.PHONY : utils/mikktspace/CMakeFiles/mikktspace.dir/build

utils/mikktspace/CMakeFiles/mikktspace.dir/requires: utils/mikktspace/CMakeFiles/mikktspace.dir/mikktspace.c.o.requires

.PHONY : utils/mikktspace/CMakeFiles/mikktspace.dir/requires

utils/mikktspace/CMakeFiles/mikktspace.dir/clean:
	cd /home/sammael/grade/dependencies/Hydra/HydraAPI/utils/mikktspace && $(CMAKE_COMMAND) -P CMakeFiles/mikktspace.dir/cmake_clean.cmake
.PHONY : utils/mikktspace/CMakeFiles/mikktspace.dir/clean

utils/mikktspace/CMakeFiles/mikktspace.dir/depend:
	cd /home/sammael/grade/dependencies/Hydra/HydraAPI && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/sammael/grade/dependencies/Hydra/HydraAPI /home/sammael/grade/dependencies/Hydra/HydraAPI/utils/mikktspace /home/sammael/grade/dependencies/Hydra/HydraAPI /home/sammael/grade/dependencies/Hydra/HydraAPI/utils/mikktspace /home/sammael/grade/dependencies/Hydra/HydraAPI/utils/mikktspace/CMakeFiles/mikktspace.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : utils/mikktspace/CMakeFiles/mikktspace.dir/depend
