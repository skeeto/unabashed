# getopts: clustering, arguments, silent mode, OPTERR and odd OPTIND
exec 2>&1
show() { echo "rc=$? o=$o OPTIND=$OPTIND OPTARG=${OPTARG-unset}"; }
set -- -a -bval -c -- -d rest
while getopts ab:c o; do show; done; show
OPTIND=1; set -- -x -b
while getopts ab: o; do show; done; show
OPTIND=1; set -- -x -b
while getopts :ab: o; do show; done; show
OPTIND=1; set -- -abc arg
while getopts ab:c o; do show; done; show
OPTIND=1; set -- - -a
getopts a o; show
OPTIND=1; set -- -a- 
getopts a o; show; getopts a o; show
OPTIND=1; getopts a: o -a; show
OPTIND=1; getopts a: o -a ''; show
OPTIND=1; getopts 'a:' o -a "" x; show
OPTIND=1; getopts '?' o '-?'; show
OPTIND=1; getopts ':' o '-:'; show
OPTIND=1; getopts 'a:' o '-a:'; show
OPTIND=1; OPTERR=0; getopts a o -x; show; unset OPTERR
OPTIND=1; getopts a o -x; show
OPTIND=0; getopts a o -a; show
OPTIND=-3; getopts a o -a; show
OPTIND=x; getopts a o -a; show
OPTIND=1; getopts; echo $?
OPTIND=1; getopts a; echo $?
OPTIND=1; getopts a 1x -a; echo $?
OPTIND=1; getopts a o -a -a -a; getopts a o -a -b; show
f() { getopts a o; show; }; OPTIND=1; set -- -a -a; f; f; f
OPTIND=1; set -- -ab; getopts a o; show; OPTIND=2; getopts ab o; show
OPTIND=1; getopts ab o -b -a; show; getopts ab o -a -b -a; show
OPTIND=1; getopts -- a o -a; echo $?
OPTIND=1; getopts -a o -a; echo $?
POSIXLY_CORRECT=1; OPTIND=1; getopts a o -x; show
