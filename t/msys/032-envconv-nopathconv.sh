# vos
# MSYS_NO_PATHCONV and the environment: which variables still convert
export FOO=/c/x
export ARGV_SHOW='FOO HOME TEMP PATH MSYS_NO_PATHCONV MSYS2_ARG_CONV_EXCL'
MSYS_NO_PATHCONV=1 argv /c/y
MSYS2_ARG_CONV_EXCL='*' argv /c/y
