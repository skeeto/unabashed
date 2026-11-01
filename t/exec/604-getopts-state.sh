# getopts keeps its place inside a word of bundled options; assigning
# OPTIND restarts it, and a function's local OPTIND restores it
set -- -abc -d
getopts abcd o; echo $o $OPTIND
getopts abcd o; echo $o $OPTIND
OPTIND=$OPTIND; getopts abcd o; echo $o $OPTIND
f() {
    local OPTIND=1 opt
    while getopts ":abcxyz" opt; do
        echo "opt: $opt $OPTIND"
        if [[ $opt = y ]]; then f -abc; fi
    done
}
f -xyz
set -- a
OPTIND=5; getopts ab o; echo $? $o $OPTIND
getopts -- ab o; echo $?
getopts -a ab o; echo $?
set -- -a -c
OPTIND=1; getopts ab opt-var; echo $? $OPTIND
OPTERR=0; getopts ab o; echo $? $o ${OPTARG-unset}
unset OPTERR
OPTIND=1; set -- -x
readonly OPTARG
getopts :x x; echo $? x=$x
getopts x x; echo $? ${OPTARG-unset}
readonly V; OPTIND=1; getopts x V; echo $? $OPTIND
