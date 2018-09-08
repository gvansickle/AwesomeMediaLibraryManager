#!/bin/sh

# Wrapper to set environment vars prior to debugging.
# Under Qt Creator:
# - pwd == the bin directory containing the built and installed exe.  prefix.sh is one level above that.
# - Add these gdb commands to the kit (kind of gross, but haven't found a better way yet) in
#   "Options"->"Debugger"->"Additional startup commands":
#      show exec-wrapper
#      set exec-wrapper '%{CurrentProject:Path}/gdb_wrap.sh'

pwd
source ../prefix.sh

exec "$@"
