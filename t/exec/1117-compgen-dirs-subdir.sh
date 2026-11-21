# Directory completion inside a directory stats each match by a path
# built as the stat is called, in the arena the stat copies: the path
# was overwritten, so no match was a directory.
mkdir -p sub/dd sub/ee/ff; : >sub/gg; : >sub/ee/hh
compgen -d sub/ | sort; echo "status $?"
compgen -d sub/e | sort
compgen -A directory sub/ee/ | sort
compgen -d s
compgen -d nosuch/; echo "none $?"
