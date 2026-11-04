# vos
# The install root through cygpath: the shell's view, as children see it
cygpath -w /
cygpath -m /
cygpath -m /usr
cygpath -m /etc
cygpath -u "$(cygpath -m /)"
cygpath -u "$(cygpath -w /)usr"
cygpath -u "$(cygpath -m /usr/bin)"
[ "$(cd / && pwd -W)/" = "$(cygpath -m /)" ] && echo same
