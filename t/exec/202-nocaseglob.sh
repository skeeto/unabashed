# nocaseglob matches without regard to case.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
cd /tmp
: > Abc; : > abd; : > ABE; : > x
show a*
shopt -s nocaseglob
show a*
show A?[CD]
show [a]*
show *E
shopt -u nocaseglob
show *E
