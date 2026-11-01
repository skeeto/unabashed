# Components without glob characters are looked up, not listed: they
# find dangling links and names in unreadable directories.
cd /tmp && mkdir gp && cd gp
mkdir -p d1/sub d2/sub d3 locked/in
touch d1/sub/f d2/sub/g locked/in/h
ln -s gone d3/sub
chmod 311 locked
show() { printf '%s:' "$1"; shift; printf ' %s' "$@"; echo; }
show '*/sub'      */sub
show '*/sub/*'    */sub/*
show 'locked/*'   locked/*
show 'locked/in/*' locked/in/*
show 'l*/in/*'    l*/in/*
show 'l*/*/h'     l*/*/h
show 'd?/sub/'    d?/sub/
show '"d1"/*/f'   "d1"/*/f
show 'd\1/*/f'    d\1/*/f
chmod 755 locked
