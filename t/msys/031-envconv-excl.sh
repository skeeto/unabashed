# vos
# MSYS2_ENV_CONV_EXCL: prefixes of whole NAME=VALUE strings, or '*'
export FOO=/c/x FOX=/c/y BAR=/c/z
export ARGV_SHOW='FOO FOX BAR HOME'
MSYS2_ENV_CONV_EXCL=FOO argv
MSYS2_ENV_CONV_EXCL='FO;BAR=/c' argv
MSYS2_ENV_CONV_EXCL='FOO=/c/x' argv
MSYS2_ENV_CONV_EXCL='*' argv
MSYS2_ENV_CONV_EXCL=HOME argv
