# vos
# cygpath -w and -m: POSIX to Windows, backslashes or forward slashes
for p in /c /c/Windows /C/Windows/ /tmp /tmp/x /usr/bin /bin /etc/fstab \
         /usr /x /d/a/b test.sh a/b ./a ../a . /dev/null /proc; do
    echo "$p -> $(cygpath -w "$p") $(cygpath -m "$p")"
done
