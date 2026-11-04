# vos
# MSYS_NO_PATHCONV, set to anything (even empty), turns conversion off
MSYS_NO_PATHCONV=1 argv /c/x /tmp --dir=/x
MSYS_NO_PATHCONV= argv /c/x /tmp
MSYS_NO_PATHCONV=0 argv /c/x
argv /c/x
export MSYS_NO_PATHCONV=
ARGV_SHOW='MSYS_NO_PATHCONV HOME' argv /c/x
unset MSYS_NO_PATHCONV
argv /c/x
