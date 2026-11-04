# vos
# Other variables go through the argument heuristic on their way out
export FOO=/tmp/a:/usr BAR=/c/x BAZ=http://x/y QUX='C:\x' ONE=/x
export OPT=--dir=/c/x LIST='/c/a;/c/b' REL=a/b EMPTY= TILDE='~/x'
export SHELL=/usr/bin/bash LD_LIBRARY_PATH=/c/a:/c/b
ARGV_SHOW='FOO BAR BAZ QUX ONE OPT LIST REL EMPTY TILDE SHELL LD_LIBRARY_PATH' argv
