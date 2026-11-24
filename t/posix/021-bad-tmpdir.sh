# A here-document too big for a pipe goes to a temporary file in TMPDIR,
# but as bash's sh_get_tmpdir, only if TMPDIR is a directory it can
# write: else the system's (/tmp)
x=$(printf '%070000d' 0)
: > notdir
for t in /nonexistent notdir "" "$PWD"; do
    TMPDIR=$t bash -c 'cat <<E | wc -c
$1
E
echo "st $?"' bash "$x"
done
