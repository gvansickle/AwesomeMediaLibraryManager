# Depends on cwd being build-kdev/install/bin
notify-send "PWD: $(pwd)";
notify-send "KDEV_BUILD_INST_DIR_FOR_RUN_DEBUG: ${KDEV_BUILD_INST_DIR_FOR_RUN_DEBUG}";
source ../../prefix.sh

# We're invoked with this, and it seems to work.
### Or maybe this:
### AwesomeMediaLibraryManager/src/kde_gdb_wrapper.sh gdb AwesomeMediaLibraryManager/src/kde_gdb_wrapper.sh gdb --interpreter=mi2 -quiet
#gdb --interpreter=mi2
exec $@
