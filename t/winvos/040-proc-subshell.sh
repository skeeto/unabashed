# personality: msys
# Hand-checked baseline: /proc/$BASHPID and its fd directory name the
# subshell. (On Windows a subshell's BASHPID is often 10 digits, which
# a /proc path also accepts; the vos's ids are shorter.)
mkdir w && cd w
echo hello >f
(
    [ -d /proc/$BASHPID ] && echo dir
    exec 3<f
    read -r l </proc/$BASHPID/fd/3 && echo "$l"
)
