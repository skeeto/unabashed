# B12: here-documents, to a program (cat) and to a builtin (read), with
# a 2 KB and a 32 KB expanded body (both fit a pipe)
printf -v s2 '%2048s' ''; s2=${s2// /x}
printf -v s32 '%32768s' ''; s32=${s32// /y}
for ((i=0;i<2000;i++)); do cat <<EOF >/dev/null
$s2 $i
EOF
done
for ((i=0;i<10000;i++)); do read -r -d '' x <<EOF
$s2 $i
EOF
done; echo ${#x}
for ((i=0;i<500;i++)); do cat <<EOF >/dev/null
$s32 $i
EOF
done
for ((i=0;i<2000;i++)); do read -r -d '' x <<EOF
$s32 $i
EOF
done; echo ${#x}
