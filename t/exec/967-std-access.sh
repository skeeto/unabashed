# The standard descriptors a shell starts with are usable as they are
# open, not only in their usual direction: a file open for reading and
# writing as standard output can be read, and standard input open only
# for reading still refuses writes
printf 'abc\n' >f
bash -c 'read x <&1; echo "read [$x]" >&2' 1<>f
bash -c 'echo more >&0; echo "st $?" >&2; cat <&0 >&2' 0<>f
bash -c 'echo x >&0; echo "st $?"' 2>/dev/null <f
