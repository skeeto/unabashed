# The physical directory follows symbolic links: pwd -P, cd -P, set -P
# and set -o physical, where pwd -P once printed the logical name and
# cd -P kept it.
mkdir -p real/sub
ln -s real link
ln -s ../real/sub real/up
here=$PWD
r() { echo "$1: pwd=${PWD#$here} P=$(pwd -P | sed "s|^$here||")"; }
cd link; r "cd link"
echo "-LP $(pwd -LP | sed "s|^$here||") -PL $(pwd -PL | sed "s|^$here||")"
cd sub; r "cd sub"
cd ..; r "cd .."
cd -P .; r "cd -P ."
cd "$here/link/sub"; cd -P ..; r "cd -P .. from link/sub"
cd "$here/link/up"; r "two links"
cd -P "$here/link/up/.."; r "cd -P through two links"
set -P; cd "$here/link"; r "set -P"; pwd -L | sed "s|^$here||"; set +P
set -o physical; cd "$here/link/sub"; r "set -o physical"; set +o physical
cd "$here/link"
(set -o posix; pwd -P >/dev/null; echo "posix PWD ${PWD#$here}")
pwd -P >/dev/null; echo "PWD ${PWD#$here}"
cd "$here"; ln -s loop loop
cd -P loop/x 2>/dev/null; echo "loop $?"
