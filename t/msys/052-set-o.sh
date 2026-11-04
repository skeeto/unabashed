# vos
# The option lists in msys mode, igncr included
set -o
set +o
echo "$-"
set -o igncr; echo "status $?"
[[ $(set -o) == *igncr*on* ]] && echo listed
