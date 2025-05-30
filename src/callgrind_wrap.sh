#!/bin/sh

# Wrapper to set environment vars prior to debugging.
# Under Qt Creator:
# - pwd == the bin directory containing the built and installed exe.  prefix.sh is one level above that.
# - Add these gdb commands to the kit (kind of gross, but haven't found a better way yet) in
#   "Options"->"Debugger"->"Additional startup commands":
#      show exec-wrapper
#      set exec-wrapper '%{CurrentProject:Path}/gdb_wrap.sh'

# VS Code notes
# Same story, but you have to get the Qt/KF pretty printers set up.
# There's these:
# @link https://github.com/Lekensteyn/qt5printers
# But kdevelop has its own in it's source tree at kdevelop/plugins/gdb/printers/:
# - kde.py
# - qt.py
# - helper.py
# - gdbinit

echo "PWD: $(pwd)"
echo "SOURCING PREFIX.SH SCRIPT"
source ../prefix.sh
echo "PATH=${PATH}";
echo "SOURCED, STARTING VALGRIND, INCOMING OPTIONS: ${@}"
#--smc-check=stack --tool=callgrind --instr-atstart=no --collect-systime=yes
/usr/bin/valgrind "${@}"
