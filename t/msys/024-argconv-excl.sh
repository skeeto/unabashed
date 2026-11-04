# vos
# MSYS2_ARG_CONV_EXCL: ';'-separated prefixes of whole arguments, or '*'
MSYS2_ARG_CONV_EXCL='*' argv /c/x /tmp
MSYS2_ARG_CONV_EXCL=/c argv /c/x /cx /tmp
MSYS2_ARG_CONV_EXCL='--dir=;-I' argv --dir=/x -I/y -L/z
MSYS2_ARG_CONV_EXCL=';;' argv /tmp
MSYS2_ARG_CONV_EXCL= argv /tmp
export MSYS2_ARG_CONV_EXCL=/tmp
argv /tmp /tmpx /c/x
ARGV_SHOW=MSYS2_ARG_CONV_EXCL argv /tmp
