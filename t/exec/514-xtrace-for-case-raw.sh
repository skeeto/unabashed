v="1 2"
w='x y'
set -- p 'q r'
set -x
for i in $v "$w"; do :; done
for j; do :; done
for k in "$@"; do :; done
case $w in x*) : ;; esac
case "$v" in *) ;; esac
for l in; do :; done
