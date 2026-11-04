# vos
# cygpath -a makes relative paths absolute against the current directory
mkdir sub
cygpath -a sub
cygpath -am .
cygpath -aw ..
cygpath -a 'sub\x'
cygpath -am sub/../test.sh
cygpath -ua 'C:\Windows'
cygpath -wa /c/Windows/..
