# Patterns with slashes: directory components and trailing slashes.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
cd /tmp
: > f1; : > f2
show */
show /home/*/
show /home/*
show /hom?/orac*
show /home/oracle/
show /tmp/f*
show /tmp//f1
show /tmp/f1/
show f*/
show ../tmp/f?
show /tmp/../tmp/f2
show /*mp/f1
show /home/*/nothere
show nodir/*
show f1/*
show ./*
show .//f1
