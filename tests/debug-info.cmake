cmake_minimum_required(VERSION 3.16)
file(MAKE_DIRECTORY "${WORK}/debug-info")
set(binary "${WORK}/debug-info/program")
execute_process(COMMAND "${COMPILER}" build "${SOURCE}/examples/debugging.ar" --debug -o "${binary}"
  RESULT_VARIABLE status OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 60)
if(NOT status EQUAL 0)
  message(FATAL_ERROR "debug build failed: ${out} ${err}")
endif()
# Loading a target and resolving breakpoints requires no debugger process launch.
execute_process(COMMAND "${LLDB}" --no-lldbinit --batch
  -o "breakpoint set --file debugging.ar --line 4" -- "${binary}"
  RESULT_VARIABLE status OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 30)
if(NOT status EQUAL 0 OR NOT out MATCHES "Breakpoint 1: [1-9]" OR out MATCHES "no locations")
  message(FATAL_ERROR "source breakpoint did not resolve after staging cleanup: ${out} ${err}")
endif()
