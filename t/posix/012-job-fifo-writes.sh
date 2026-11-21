# A background job's descriptor that is a FIFO is written as one, though
# in the shell that started it the same number was a file it wrote to
# (here the here-document's): once the job took it for the file and
# held its writes back until it ended.
mkfifo p
big=$(printf '%100000s' '')
{
    { exec 3>p; echo ready >&3; echo again >&3; s=$SECONDS; while ((SECONDS - s < 4)); do :; done; } &
} <<EOF
$big
EOF
exec 4<p
read -r -t 2 a <&4; read -r -t 2 b <&4
echo "read: $a $b"
kill $!; wait $!; echo "status $?"
