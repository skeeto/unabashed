# vos
# CDPATH splits on ':' in msys mode, so a Windows path in it breaks
mkdir -p a/b c/d
here=$PWD
CDPATH=$here/a:$here/c
cd b && echo "${PWD#"$here"}"
cd d && echo "${PWD#"$here"}"
cd "$here"
CDPATH=$(pwd -W)/a
cd b; echo "status $? ${PWD#"$here"}"
cd "$here"
CDPATH=:a
cd b >/dev/null && echo "${PWD#"$here"}"
