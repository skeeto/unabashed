# A last line ending in a backslash: kept, but an end-of-file error
# counts the line bash looked for past it
exec 2>&1
eval 'echo a\'
eval 'echo $LINENO\'
eval 'echo $LINENO \'
eval 'echo $LINENO; \'
eval 'echo $LINENO
\'
eval 'f() { :\'
eval 'g() { :; \'
bash -c 'echo b\'
bash -c 'echo $LINENO \'
echo 'echo c\' > zzq.sh; bash zzq.sh; printf 'echo d\\' > zzq.sh; bash zzq.sh; printf 'h() { \\' > zzq.sh; bash zzq.sh; rm zzq.sh
