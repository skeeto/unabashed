# DEBUG and RETURN in sourced files follow the function rules: only
# their own traps run in them, unless set -T; RETURN runs where the
# file was sourced; exit inside a sourced file leaves no stale frame
# (unlike bash, the EXIT action then runs with the frames gone)
printf 'echo s1\necho s2\n' > sub1.sh
printf 'trap "echo d2 \$LINENO" DEBUG\necho t1\necho t2\n' > sub2.sh
printf 'f() { echo "in f: ${FUNCNAME[*]} ${BASH_SOURCE[*]}"; }\nf\nexit 3\n' > sub3.sh
bash -c 'trap "echo debug \$LINENO" DEBUG; . ./sub1.sh; echo end'
echo ---
bash -c 'trap "echo debug \$LINENO" DEBUG; set -T; . ./sub1.sh; echo end'
echo ---
bash -c 'trap "echo ret \$LINENO \${FUNCNAME[0]-x} \${BASH_SOURCE[0]-x}" RETURN
. ./sub1.sh; echo end'
echo ---
bash -c '. ./sub2.sh; echo end'
echo ---
bash -c 'trap "echo debug \$LINENO" DEBUG; trap "echo ret \$LINENO" RETURN; f() { . ./sub1.sh; }; f; set -T; f'
echo ---
bash -c 'trap "echo exit: \$LINENO" EXIT; g() { . ./sub3.sh; }; g'
echo "st $?"
