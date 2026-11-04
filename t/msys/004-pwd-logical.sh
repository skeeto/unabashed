# vos
# PWD, OLDPWD and pwd -L/-P through ., .. and repeated slashes
mkdir -p a/b
here=$PWD
cd a//b/ && pwd && echo "$PWD"
cd "$here" && cd ./a/./b/.. && pwd
cd "$here" && cd a/b/../../a && pwd -L && pwd -P
cd "$here" && cd a && echo "${OLDPWD#"$here"}" "${PWD#"$here"}"
cd "$here" && cd a/missing 2>/dev/null; echo "status $? ${PWD#"$here"}"
cd "$here"; PWD=/c; pwd; cd .; pwd
