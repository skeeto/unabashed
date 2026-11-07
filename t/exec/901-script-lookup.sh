# A script named without a slash and not in the working directory is
# looked for along PATH, executable or not, and messages then use the
# found name; a #! line naming no file is a "bad interpreter", a
# trailing CR shown as ^M; outside POSIX mode a function name may have
# a slash, and export -f refuses names bash cannot pass on
mkdir bin
printf 'echo in script $0 "$@"\n' > bin/scr
PATH=$PWD/bin:$PATH
bash scr a b
bash cat 2>&1 | sed "s|$PWD|PWD|g"; echo "st ${PIPESTATUS[0]}"
bash nosuchscript; echo "st $?"
bash -- scr x

printf '#! nosuch arg\necho x\n' > ns
chmod +x ns
./ns; echo "st $?"
bash ./ns
printf '#!/no/sh\r\necho x\n' > crs
chmod +x crs
./crs; echo "st $?"

function /x { echo fn; }
/x
type /x | head -1
(set -o posix; /x; echo "st $?")
function a=b { echo ab; }
export -f a=b; echo "st $?"
export -nf a=b; echo "st $?"
function c/d { :; }
export -f c/d; echo "st $?"
