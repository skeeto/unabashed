# Runs inside the bash 5.3 oracle container (see test_record.sh): builds
# a small file tree in a fresh temp directory, dumps its metadata, then
# runs each case from the file named by $1 and prints the case followed
# by "STATUS[ MESSAGE]", with the "bash: line N: test: " prefix removed.
# This is free and unencumbered software released into the public domain.

cases=$1
bash=/usr/local/bin/bash
cd "$(mktemp -d)" || exit 1

printf 'hello\n' >reg
: >empty
mkdir dir
ln -s reg link
ln -s nowhere dlink
ln -s dir dirlink
mkfifo fifo
perl -MSocket -e 'socket(S, PF_UNIX, SOCK_STREAM, 0) &&
                  bind(S, sockaddr_un("sock")) or exit 1' || exit 1
for m in 000 644 755 444 200 111 4755 2755; do
    printf x >m$m
done
printf x >old
printf x >new
printf x >atime
mkdir sticky dir000
ln reg hard
touch -h -d '2010-01-01 00:00:00' *
touch -d '2001-01-01 00:00:00' old
touch -d '2020-01-01 00:00:00' new
touch -d '2015-01-01 00:00:00' atime
touch -a -d '2005-01-01 00:00:00' atime
for m in 000 644 755 444 200 111 4755 2755; do
    chmod $m m$m
done
chmod 1777 sticky
chmod 000 dir000

# Metadata as "@stat NAME FOLLOW TYPE MODE UID GID SIZE MTIME ATIME DEV INO"
dump() {
    if [ "$2" = 1 ]; then
        out=$(stat -L -c '%F|%a|%u|%g|%s|%Y|%X|%d|%i' -- "$1" 2>/dev/null)
    else
        out=$(stat -c '%F|%a|%u|%g|%s|%Y|%X|%d|%i' -- "$1" 2>/dev/null)
    fi
    if [ -z "$out" ]; then
        echo "@stat $1 $2 none"
        return
    fi
    type=${out%%|*}
    case $type in
    'regular file'|'regular empty file') type=reg ;;
    directory)                           type=dir ;;
    'symbolic link')                     type=lnk ;;
    fifo)                                type=fifo ;;
    socket)                              type=sock ;;
    'character special file')            type=chr ;;
    'block special file')                type=blk ;;
    *)                                   type=other ;;
    esac
    rest=${out#*|}
    echo "@stat $1 $2 $type $(printf '%s' "$rest" | tr '|' ' ')"
}
for f in * . reg/ dir/ dirlink/ link/ dir000/ sticky/ dlink/ fifo/ \
         missing missing2 / /tmp /dev/null /etc/passwd /usr/local/bin/bash /nonexistent; do
    dump "$f" 1
    dump "$f" 0
done
echo "@id $(id -u) $(id -g)"

setup='x=1; empty=; arr=(a b); declare -A as=([k]=v); declare -n ref=x badref=nowhere'
while IFS= read -r line; do
    case $line in
    ''|'# '*) continue ;;
    '#posix '*) pre="$setup; set -o posix"; src=${line#'#posix '} ;;
    *) pre=$setup; src=$line ;;
    esac
    err=$(LANG=C $bash --norc --noprofile -c "$pre
$src" 2>&1 >/dev/null </dev/null)
    st=$?
    err=$(printf '%s' "$err" | sed 's/^[^:]*: line [0-9]*: \(test\|\[\): //')
    case $err in
    *'
'*) err="MULTILINE $err" ;;
    esac
    printf '%s\n%s\n' "$line" "$st${err:+ $err}"
done <"$cases"
