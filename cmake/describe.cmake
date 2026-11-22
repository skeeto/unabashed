# Write OUT, unabashed_describe.h, with `git describe --dirty` of SRC
# for src/version.cpp, only when it changed, so that a build recompiles
# only after a commit or an edit. Outside a checkout (a tarball, whose
# version.cpp git archive filled in) OUT is removed instead.
#   cmake -DSRC=dir -DOUT=file -P cmake/describe.cmake
# This is free and unencumbered software released into the public domain.
# Only SRC's own repository: a tarball unpacked inside another one
# (a host's tree) names no commit of that one.
execute_process(
  COMMAND git rev-parse --show-toplevel
  WORKING_DIRECTORY ${SRC}
  OUTPUT_VARIABLE top OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE status ERROR_QUIET)
set(describe "")
if(status EQUAL 0)
  file(REAL_PATH ${top} top)
  file(REAL_PATH ${SRC} src)
  if(top STREQUAL src)
    execute_process(
      COMMAND git describe --dirty --always
      WORKING_DIRECTORY ${SRC}
      OUTPUT_VARIABLE describe OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE status ERROR_QUIET)
  endif()
endif()
if(status EQUAL 0 AND describe)
  set(text "#define UNABASHED_DESCRIBE \"${describe}\"\n")
  set(old "")
  if(EXISTS ${OUT})
    file(READ ${OUT} old)
  endif()
  if(NOT old STREQUAL text)
    file(WRITE ${OUT} "${text}")
  endif()
else()
  file(REMOVE ${OUT})
endif()
