/tmp
echo "dir st=$?"
echo 'echo hi' > /tmp/notexec
/tmp/notexec
echo "noexec st=$?"
cd /tmp
./notexec
echo "rel st=$?"
/tmp/notexec/sub
echo "notdir st=$?"
command nosuch
echo "command st=$?"
exec nosuch
echo notreached
