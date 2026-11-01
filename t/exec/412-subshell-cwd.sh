# cd inside a subshell does not change the parent cwd
pwd
(cd /tmp && pwd)
pwd
x=$(cd /; pwd)
echo "$x"
pwd
(cd /tmp; echo "$PWD $OLDPWD")
echo "$PWD ${OLDPWD-unset}"
(cd /nonexistent 2>&1; echo "st=$?")
pwd
