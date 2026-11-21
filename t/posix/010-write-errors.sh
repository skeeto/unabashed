# A builtin's failed write to a file is reported by that builtin, with
# the system's reason, as bash's is once it flushes its stdout, though
# the shell gathers small writes to a file: here past a file size limit,
# SIGXFSZ ignored. Once, the failure of a lone printf showed only as
# the file was closed, and was lost.
trap '' XFSZ
ulimit -f 1
printf '%2000s\n' x >f; echo "printf $?"
exec 3>g
echo a >&3; echo b >&3; printf '%2000s\n' x >&3; echo "third $?"
exec 3>&-
fails=0
for ((i = 1; i <= 2000; i++)); do echo $i || ((fails++)); done >h 2>/dev/null
lines=$(wc -l <h)
echo "each echo wrote its line or failed: $((lines + fails == 2000))"
echo "some failed: $((fails > 0))"
