# A level killed while it waits for a program whose output goes into a
# shell pipe: the program runs on, holding the pipe open
{ cat; echo "done $?"; } < <(p=$BASHPID; { sleep 1; kill $p; } & seq 2; sleep 2; echo unreached)
x=$(p=$BASHPID; { sleep 1; kill $p; } & seq 3 | cat; sleep 2; echo unreached); echo "[$x]"
