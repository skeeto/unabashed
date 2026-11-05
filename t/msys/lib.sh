# t/msys/lib.sh: shared by t/msys/run and tools/gbrecord (sourced)
#
# Callers set root (the repository) and work (WORKDIR, absolute), then
# call msys_init SHELL. GITBASH names Git Bash's real shell, whose
# install lends the MSYS2 programs. Runs under w64devkit's sh. See
# tools/README.md, "Git Bash differential tests", for the layout, the
# environment and the normalization rules.

gitbash=${GITBASH:-C:/Users/wellons/git/usr/bin/bash.exe}

# msys_abs DIR: DIR's absolute path as C:/x with an uppercase drive
msys_abs() {
    p=$(cd "$1" && pwd) || return 1
    case $p in
    /?|/?/*) p=${p#/}; p=$(printf '%s' "$p" | cut -c1):${p#?} ;;
    esac
    printf '%s%s\n' "$(printf '%s' "$p" | cut -c1 | tr a-z A-Z)" "${p#?}"
}

# msys_workdir DIR: create WORKDIR and print it as C:/x. Refuses /c/x:
# w64devkit's sh would take it as C:/c/x and create that.
msys_workdir() {
    case $1 in
    /[a-zA-Z]|/[a-zA-Z]/*)
        echo "$0: write WORKDIR as C:/..., not $1" >&2; exit 2 ;;
    esac
    mkdir -p "$1" || exit 2
    msys_abs "$1" || exit 2
}

# msys_gitclone: WORK/git, the Git install the corpus uses: Git's
# usr/bin hard-linked (copied when that fails) and its etc copied. An
# MSYS2 program keys its mount table by its DLL's directory, and the
# first live process of an install fixes the table for every later one
# (/tmp from that process's TMP). Git's own install is shared with any
# Git Bash the user runs, so a child of a non-MSYS2 shell would see
# their /tmp (a junction does not help: the runtime resolves it). The
# clone has a key of its own, so each test's first MSYS2 child reads
# the clone's fstab with the test's TMP; Git Bash's children inherit
# Git Bash's table either way. Rebuilt when Git's files change.
msys_gitclone() {
    gitroot=$work/git
    stamp=$gitroot/.stamp
    if [ -f "$stamp" ] && [ -f "$gitroot/usr/bin/msys-2.0.dll" ] &&
       [ -z "$(find "$gitreal/usr/bin" "$gitreal/etc" -newer "$stamp" | head -1)" ]; then
        return
    fi
    rm -rf "$gitroot"
    mkdir -p "$gitroot/usr" || exit 2
    cp -rl "$gitreal/usr/bin" "$gitroot/usr/" 2>/dev/null ||
        { rm -rf "$gitroot/usr/bin"; cp -r "$gitreal/usr/bin" "$gitroot/usr/"; } ||
        exit 2
    cp -r "$gitreal/etc" "$gitroot/" || exit 2
    : >"$stamp"
}

# msys_init SHELL: build WORK/bin and the scratch install WORK/inst
msys_init() {
    case $work in
    "$root"|"$root"/*)
        echo "$0: WORKDIR must be outside the repository" >&2; exit 2 ;;
    esac
    if [ ! -f "$gitbash" ]; then
        echo "$0: no Git Bash at $gitbash (set GITBASH)" >&2
        exit 2
    fi
    gitreal=$(msys_abs "$(dirname "$gitbash")/../..") || exit 2
    bin=$work/bin
    inst=$work/inst
    mkdir -p "$bin" || exit 2
    msys_gitclone
    if [ ! -e "$bin/sandbox.exe" ] || [ "$root/tools/sandbox.cpp" -nt "$bin/sandbox.exe" ]; then
        cc -nostartfiles -o "$bin/sandbox.exe" "$root/tools/sandbox.cpp" \
           -ladvapi32 || exit 2
    fi
    for v in argv:g++ argv32:i686-w64-mingw32-g++; do
        exe=$bin/${v%%:*}.exe
        if [ ! -e "$exe" ] || [ "$root/tools/argv.cpp" -nt "$exe" ]; then
            ${v#*:} -std=c++20 -municode -O2 -s -o "$exe" \
                "$root/tools/argv.cpp" || exit 2
        fi
    done

    # A fresh install tree rooted at WORK/inst: the shell under test as
    # usr/bin/bash.exe, and an fstab lending it Git's programs and
    # configuration. An MSYS2 program finds its root from msys-2.0.dll,
    # so a copied Git Bash needs a copy of that beside it.
    rm -rf "$inst"
    mkdir -p "$inst/usr/bin" "$inst/etc" || exit 2
    cp "$1" "$inst/usr/bin/bash.exe" || exit 2
    dll=$(dirname "$1")/msys-2.0.dll
    if [ -f "$dll" ]; then
        cp "$dll" "$inst/usr/bin/" || exit 2
    fi
    g=$(printf '%s' "$gitroot" | sed 's/ /\\040/g')
    {
        echo "# t/msys scratch install: Git's programs, the shell under test"
        echo "$g/usr/bin /usr/bin ntfs binary,noacl,auto 0 0"
        echo "$g/usr/bin /bin ntfs binary,noacl,auto 0 0"
        echo "$g/etc /etc ntfs binary,noacl,auto 0 0"
        echo "none / cygdrive binary,posix=0,noacl,user 0 0"
        echo "none /tmp usertemp binary,posix=0,noacl 0 0"
    } >"$inst/etc/fstab"

    sysroot=${SYSTEMROOT:-'C:\Windows'}
    w64=$(msys_abs "$(dirname "$(command -v cc)")/..") || exit 2
    msys_normscript >"$work/norm.sed"
}

# msys_spellings TOKEN DIR: "LEN TOKEN LITERAL" for each spelling of DIR
msys_spellings() {
    p=$(msys_abs "$2") || exit 2
    d=$(printf '%s' "$p" | cut -c1)
    lo=$(printf '%s' "$d" | tr A-Z a-z)
    rest=${p#?:}
    back=$(printf '%s' "$rest" | tr / '\\')
    for s in "$d:$rest" "$lo:$rest" "$d:$back" "$lo:$back" \
             "/$lo$rest" "/$d$rest"; do
        printf '%d %s %s\n' ${#s} "$1" "$s"
    done
}

# msys_normscript: a sed script replacing every spelling, longest first
msys_normscript() {
    {
        msys_spellings @ROOT@ "$inst"
        msys_spellings @WORK@ "$work"
        msys_spellings @GIT@ "$gitroot"
        msys_spellings @W64@ "$w64"
    } | sort -k1,1nr | while read -r len token lit; do
        esc=$(printf '%s' "$lit" | sed 's/[]\\.*^$[|]/\\&/g')
        printf 's|%s|%s|g\n' "$esc" "$token"
    done
}

# msys_run SCRIPT OUT ERR MERGE: run SCRIPT as test.sh in a fresh
# WORK/NAME, its output and errors apart (MERGE 0) or as one stream in
# OUT (MERGE 1, ERR then empty), both normalized; sets status.
msys_run() {
    script=$1 out=$2 err=$3 merge=$4
    name=$(basename "$script" .sh)
    dir=$work/$name
    rm -rf "$dir"
    mkdir -p "$dir/home" "$dir/tmp" || exit 2
    cp "$script" "$dir/test.sh" || exit 2
    # NAME.d: files laid over the test directory (home/.profile, ...)
    tree=${script%.sh}.d
    if [ -d "$tree" ]; then
        cp -r "$tree/." "$dir/" || exit 2
    fi
    # With an etc of its own, the test gets a private install whose
    # /etc is that directory, so no file of Git's etc is read
    shellexe=$inst/usr/bin/bash.exe
    if [ -d "$dir/etc" ]; then
        mkdir -p "$dir/inst/usr/bin" "$dir/inst/etc" || exit 2
        cp "$inst/usr/bin/"* "$dir/inst/usr/bin/" || exit 2
        e=$(msys_abs "$dir/etc" | sed 's/ /\\040/g')
        sed "s|^[^ ]* /etc |$e /etc |" "$inst/etc/fstab" \
            >"$dir/inst/etc/fstab" || exit 2
        shellexe=$dir/inst/usr/bin/bash.exe
    fi
    # D, DW and DU: the test directory as C:/x, C:\x and /c/x, for
    # "# shell:" arguments
    D=$(msys_abs "$dir")
    DW=$(printf '%s' "$D" | tr / '\\')
    DU=/$(printf '%s' "$D" | cut -c1 | tr A-Z a-z)${D#?:}
    args=$(sed -n 's/^# shell: //p' "$script")
    if [ -n "$args" ]; then
        eval "set -- $args"
    else
        set -- --norc --noprofile test.sh
    fi
    path=$(printf '%s' "$gitroot/usr/bin;$w64/bin;$sysroot/System32;$bin" |
           tr / '\\')
    # "# env: NAME=VALUE" lines add variables (no blanks; $D, $DW and
    # $DU expanded), and "# env: -TZ" drops TZ=UTC
    tz=TZ=UTC
    xenv=
    for e in $(sed -n 's/^# env: //p' "$script"); do
        case $e in
        -TZ) tz= ;;
        *)   eval "e=\"$e\""; xenv="$xenv -E $e" ;;
        esac
    done
    set -- "$bin/sandbox.exe" -L -t "${MSYS_TIMEOUT:-60}" -p 64 \
        -E "PATH=$path" -E "SYSTEMROOT=$sysroot" \
        -E "COMSPEC=$sysroot\\System32\\cmd.exe" \
        -E "HOME=$DW\\home" -E "TEMP=$DW\\tmp" -E "TMP=$DW\\tmp" \
        -E LANG=C ${tz:+-E "$tz"} ${msys_xenv:+-E "$msys_xenv"} \
        $xenv "$dir" -- "$shellexe" "$@"
    if [ "$merge" = 1 ]; then
        "$@" >"$dir/raw.out" 2>&1 </dev/null
        status=$?
        : >"$dir/raw.err"
    else
        "$@" >"$dir/raw.out" 2>"$dir/raw.err" </dev/null
        status=$?
    fi
    # -b: busybox-w32's sed otherwise drops the CR of each CRLF
    sed -b -f "$work/norm.sed" "$dir/raw.out" >"$out"
    sed -b -f "$work/norm.sed" "$dir/raw.err" >"$err"
}

# msys_block NAME FILE: one byte-counted spec block
msys_block() {
    printf '== %s %d\n' "$1" $(($(wc -c <"$2")))
    cat "$2"
    printf '\n'
}

# msys_spec SCRIPT SPEC: run SCRIPT apart and merged, write SPEC; fails
# if the two statuses differ
msys_spec() {
    msys_run "$1" "$work/spec.out" "$work/spec.err" 0
    apart=$status
    msys_run "$1" "$work/spec.all" "$work/spec.none" 1
    if [ "$status" != "$apart" ]; then
        echo "$1: status $apart apart, $status merged" >&2
        return 1
    fi
    {
        msys_block stdout "$work/spec.out"
        msys_block stderr "$work/spec.err"
        msys_block merged "$work/spec.all"
        printf '== status %d\n' "$status"
    } >"$2"
}
