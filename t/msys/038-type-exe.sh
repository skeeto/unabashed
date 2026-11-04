# type, command -v and hash show programs without their .exe
type ls cat
command -v ls
command -V ls
type -P ls
type -p sed
type -t ls
type ls.exe
command -v ls.exe
which() { type -P "$@"; }
which grep
hash ls
hash -t ls
hash
hash -r
type -a printf
type -a sh
