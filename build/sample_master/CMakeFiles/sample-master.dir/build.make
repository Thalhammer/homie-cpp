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
CMAKE_SOURCE_DIR = /home/dominik/Dokumente/homie-cpp

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/dominik/Dokumente/homie-cpp/build

# Include any dependencies generated for this target.
include sample_master/CMakeFiles/sample-master.dir/depend.make

# Include the progress variables for this target.
include sample_master/CMakeFiles/sample-master.dir/progress.make

# Include the compile flags for this target's objects.
include sample_master/CMakeFiles/sample-master.dir/flags.make

sample_master/CMakeFiles/sample-master.dir/main.cpp.o: sample_master/CMakeFiles/sample-master.dir/flags.make
sample_master/CMakeFiles/sample-master.dir/main.cpp.o: ../sample_master/main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dominik/Dokumente/homie-cpp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object sample_master/CMakeFiles/sample-master.dir/main.cpp.o"
	cd /home/dominik/Dokumente/homie-cpp/build/sample_master && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/sample-master.dir/main.cpp.o -c /home/dominik/Dokumente/homie-cpp/sample_master/main.cpp

sample_master/CMakeFiles/sample-master.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/sample-master.dir/main.cpp.i"
	cd /home/dominik/Dokumente/homie-cpp/build/sample_master && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/dominik/Dokumente/homie-cpp/sample_master/main.cpp > CMakeFiles/sample-master.dir/main.cpp.i

sample_master/CMakeFiles/sample-master.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/sample-master.dir/main.cpp.s"
	cd /home/dominik/Dokumente/homie-cpp/build/sample_master && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/dominik/Dokumente/homie-cpp/sample_master/main.cpp -o CMakeFiles/sample-master.dir/main.cpp.s

sample_master/CMakeFiles/sample-master.dir/main.cpp.o.requires:

.PHONY : sample_master/CMakeFiles/sample-master.dir/main.cpp.o.requires

sample_master/CMakeFiles/sample-master.dir/main.cpp.o.provides: sample_master/CMakeFiles/sample-master.dir/main.cpp.o.requires
	$(MAKE) -f sample_master/CMakeFiles/sample-master.dir/build.make sample_master/CMakeFiles/sample-master.dir/main.cpp.o.provides.build
.PHONY : sample_master/CMakeFiles/sample-master.dir/main.cpp.o.provides

sample_master/CMakeFiles/sample-master.dir/main.cpp.o.provides.build: sample_master/CMakeFiles/sample-master.dir/main.cpp.o


sample_master/CMakeFiles/sample-master.dir/console.cpp.o: sample_master/CMakeFiles/sample-master.dir/flags.make
sample_master/CMakeFiles/sample-master.dir/console.cpp.o: ../sample_master/console.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dominik/Dokumente/homie-cpp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object sample_master/CMakeFiles/sample-master.dir/console.cpp.o"
	cd /home/dominik/Dokumente/homie-cpp/build/sample_master && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/sample-master.dir/console.cpp.o -c /home/dominik/Dokumente/homie-cpp/sample_master/console.cpp

sample_master/CMakeFiles/sample-master.dir/console.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/sample-master.dir/console.cpp.i"
	cd /home/dominik/Dokumente/homie-cpp/build/sample_master && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/dominik/Dokumente/homie-cpp/sample_master/console.cpp > CMakeFiles/sample-master.dir/console.cpp.i

sample_master/CMakeFiles/sample-master.dir/console.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/sample-master.dir/console.cpp.s"
	cd /home/dominik/Dokumente/homie-cpp/build/sample_master && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/dominik/Dokumente/homie-cpp/sample_master/console.cpp -o CMakeFiles/sample-master.dir/console.cpp.s

sample_master/CMakeFiles/sample-master.dir/console.cpp.o.requires:

.PHONY : sample_master/CMakeFiles/sample-master.dir/console.cpp.o.requires

sample_master/CMakeFiles/sample-master.dir/console.cpp.o.provides: sample_master/CMakeFiles/sample-master.dir/console.cpp.o.requires
	$(MAKE) -f sample_master/CMakeFiles/sample-master.dir/build.make sample_master/CMakeFiles/sample-master.dir/console.cpp.o.provides.build
.PHONY : sample_master/CMakeFiles/sample-master.dir/console.cpp.o.provides

sample_master/CMakeFiles/sample-master.dir/console.cpp.o.provides.build: sample_master/CMakeFiles/sample-master.dir/console.cpp.o


sample_master/CMakeFiles/sample-master.dir/mqtt_client.cpp.o: sample_master/CMakeFiles/sample-master.dir/flags.make
sample_master/CMakeFiles/sample-master.dir/mqtt_client.cpp.o: ../sample_master/mqtt_client.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dominik/Dokumente/homie-cpp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object sample_master/CMakeFiles/sample-master.dir/mqtt_client.cpp.o"
	cd /home/dominik/Dokumente/homie-cpp/build/sample_master && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/sample-master.dir/mqtt_client.cpp.o -c /home/dominik/Dokumente/homie-cpp/sample_master/mqtt_client.cpp

sample_master/CMakeFiles/sample-master.dir/mqtt_client.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/sample-master.dir/mqtt_client.cpp.i"
	cd /home/dominik/Dokumente/homie-cpp/build/sample_master && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/dominik/Dokumente/homie-cpp/sample_master/mqtt_client.cpp > CMakeFiles/sample-master.dir/mqtt_client.cpp.i

sample_master/CMakeFiles/sample-master.dir/mqtt_client.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/sample-master.dir/mqtt_client.cpp.s"
	cd /home/dominik/Dokumente/homie-cpp/build/sample_master && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/dominik/Dokumente/homie-cpp/sample_master/mqtt_client.cpp -o CMakeFiles/sample-master.dir/mqtt_client.cpp.s

sample_master/CMakeFiles/sample-master.dir/mqtt_client.cpp.o.requires:

.PHONY : sample_master/CMakeFiles/sample-master.dir/mqtt_client.cpp.o.requires

sample_master/CMakeFiles/sample-master.dir/mqtt_client.cpp.o.provides: sample_master/CMakeFiles/sample-master.dir/mqtt_client.cpp.o.requires
	$(MAKE) -f sample_master/CMakeFiles/sample-master.dir/build.make sample_master/CMakeFiles/sample-master.dir/mqtt_client.cpp.o.provides.build
.PHONY : sample_master/CMakeFiles/sample-master.dir/mqtt_client.cpp.o.provides

sample_master/CMakeFiles/sample-master.dir/mqtt_client.cpp.o.provides.build: sample_master/CMakeFiles/sample-master.dir/mqtt_client.cpp.o


# Object files for target sample-master
sample__master_OBJECTS = \
"CMakeFiles/sample-master.dir/main.cpp.o" \
"CMakeFiles/sample-master.dir/console.cpp.o" \
"CMakeFiles/sample-master.dir/mqtt_client.cpp.o"

# External object files for target sample-master
sample__master_EXTERNAL_OBJECTS =

sample_master/sample-master: sample_master/CMakeFiles/sample-master.dir/main.cpp.o
sample_master/sample-master: sample_master/CMakeFiles/sample-master.dir/console.cpp.o
sample_master/sample-master: sample_master/CMakeFiles/sample-master.dir/mqtt_client.cpp.o
sample_master/sample-master: sample_master/CMakeFiles/sample-master.dir/build.make
sample_master/sample-master: sample_master/CMakeFiles/sample-master.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/dominik/Dokumente/homie-cpp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Linking CXX executable sample-master"
	cd /home/dominik/Dokumente/homie-cpp/build/sample_master && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/sample-master.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
sample_master/CMakeFiles/sample-master.dir/build: sample_master/sample-master

.PHONY : sample_master/CMakeFiles/sample-master.dir/build

sample_master/CMakeFiles/sample-master.dir/requires: sample_master/CMakeFiles/sample-master.dir/main.cpp.o.requires
sample_master/CMakeFiles/sample-master.dir/requires: sample_master/CMakeFiles/sample-master.dir/console.cpp.o.requires
sample_master/CMakeFiles/sample-master.dir/requires: sample_master/CMakeFiles/sample-master.dir/mqtt_client.cpp.o.requires

.PHONY : sample_master/CMakeFiles/sample-master.dir/requires

sample_master/CMakeFiles/sample-master.dir/clean:
	cd /home/dominik/Dokumente/homie-cpp/build/sample_master && $(CMAKE_COMMAND) -P CMakeFiles/sample-master.dir/cmake_clean.cmake
.PHONY : sample_master/CMakeFiles/sample-master.dir/clean

sample_master/CMakeFiles/sample-master.dir/depend:
	cd /home/dominik/Dokumente/homie-cpp/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/dominik/Dokumente/homie-cpp /home/dominik/Dokumente/homie-cpp/sample_master /home/dominik/Dokumente/homie-cpp/build /home/dominik/Dokumente/homie-cpp/build/sample_master /home/dominik/Dokumente/homie-cpp/build/sample_master/CMakeFiles/sample-master.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : sample_master/CMakeFiles/sample-master.dir/depend

