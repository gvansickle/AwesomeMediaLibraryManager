#!/bin/sh

# Wrapper to set environment vars prior to debugging.
# Under Qt Creator:
# - pwd == the bin directory containing the built and installed exe.  prefix.sh is one level above that.
# - Add these gdb commands to the kit (kind of gross, but haven't found a better way yet) in
#   "Options"->"Debugger"->"Additional startup commands":
#      show exec-wrapper
#      set exec-wrapper '%{CurrentProject:Path}/gdb_wrap.sh'

# VS Code notes
# Same story, but you have to get the Qt5/KF5 pretty printers set up.
# There's these:
# @link https://github.com/Lekensteyn/qt5printers
# But kdevelop has its own in it's source tree at kdevelop/plugins/gdb/printers/:
# - kde.py
# - qt.py
# - helper.py
# - gdbinit

pwd
source ../prefix.sh

exec "$@"
