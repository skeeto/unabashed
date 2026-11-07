# fg, bg and the %N shorthand for running jobs. Without job control
# they refuse; a job started before set -m was started without it. fg
# prints the command, waits for it as a foreground command and takes
# its status; bg of a running job says it is already in the background.
# %N is fg %N, and "%N &" bg %N in this shell. Not in a stage or a job,
# which bash forks before expanding: there %N is a command not found.
# In a command substitution fg and bg find no job of their own.
bash -c 'fg; echo st $?; bg %1; echo st $?; sleep 1 & %1; echo st $?; %1 & echo st $?; fg -s; echo st $?' 2>&1
bash -c 'sleep 1 & set -m; fg; echo st $?; bg; echo st $?' 2>&1
bash -c 'set -m; fg; echo st $?; bg; echo st $?; fg %1; echo st $?; %1; echo st $?; bg %2 %3; echo st $?' 2>&1
bash -c 'set -m; fg -s; echo st $?; bg -x; echo st $?; fg --; echo st $?; fg -- %1; echo st $?' 2>&1
bash -c 'set -m; (sleep 1; exit 3) & fg; echo st $?' 2>&1
bash -c 'set -m; sleep 2 & sleep 1 & bg; echo st $?; bg %1 %2 %3; echo st $?; wait' 2>&1
bash -c 'set -m; sleep 1 | sleep 1 & %1; echo st $?' 2>&1
bash -c 'set -m; sleep 1 & %sl & echo st $?; jobs; %%; echo st $?' 2>&1
bash -c 'set -m; true & sleep 1; fg; echo st $?; jobs; fg; echo st $?' 2>&1
bash -c 'set -m; sleep 1 & fg 1; echo st $?; sleep 1 & fg %1 %2; echo st $?' 2>&1
bash -c 'set -m; sleep 1 & X=1 %+ </dev/null >/dev/null; echo st $?' 2>&1
bash -c 'set -m; sleep 1 & x=%1; $x; echo st $?' 2>&1
bash -c 'set -m; sleep 1 & %1 | cat; echo st $?; echo | %1; echo st $?; wait' 2>&1
bash -c 'set -m; sleep 1 & (%1); echo st $?; { %1; } | cat; echo "$(%1; jobs)"; f() { %1; }; f; echo st $?' 2>&1
bash -c 'set -m; sleep 1 & echo "$(fg %1; bg; fg %nope; sleep 1 & set -m; fg)"' 2>&1
bash -c 'set -m; sleep 1 & (sleep 1 & set -m; fg); (set -m; fg); echo st $?; wait' 2>&1
bash -c 'set -m; sleep 1 & fg() { echo func; }; %1; echo st $?' 2>&1
bash -c 'set -m; f() { sleep 1; }; f & fg' 2>&1
bash -c 'set -m; sleep 1 & cd /; fg' 2>&1
bash -c 'set -m; sleep 3 & p=$!; (sleep 1; kill $p) & fg %1; echo st $?; wait' 2>&1
bash -c 'set -m; trap "echo usr1" USR1; sleep 2 & (sleep 1; kill -USR1 $$) & fg %1; echo st $?; wait' 2>&1
bash -c 'set -m; sleep 1 & set -x; fg; %1; bg' 2>&1
bash -c 'set -m; sleep 1 & set +m; fg; echo st $?; wait' 2>&1
bash -c 'set -m; sleep 1 & p=$!; fg; wait $p 2>/dev/null; echo st $?' 2>&1
