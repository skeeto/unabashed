count() { echo "$#"; }
count
count ""
count "" ""
count "$@"
set -- "a b" "" c
count "$@"
count $@
count "$*"
count ${undefined}
count "${undefined}"
star() { local IFS=:; echo "[$*] [$@]"; }
star 1 2 3
at() { for a in "$@"; do echo "<$a>"; done; }
at "$@"
at "x$@y"
