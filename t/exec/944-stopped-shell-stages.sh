# Under set -m, shell code in a foreground job runs in a process of the
# job's group, as bash forks it: a pipeline stage that is a group, a
# function or ( ), and a ( ) on its own. So it stops with the job (kill
# -STOP 0 from inside stops the whole group, a stage can stop itself),
# fg continues it, and PIPESTATUS still has every stage's status.
bash -c 'set -m; { kill -STOP 0; echo resumed; } | cat; echo st=$? ${PIPESTATUS[@]}; jobs; fg; echo st=$? ${PIPESTATUS[@]}' 2>&1
bash -c 'set -m; true | { kill -TSTP $BASHPID; echo after; }; echo st=$?; jobs; fg; echo st=$?' 2>&1
bash -c 'set -m; ( kill -STOP $BASHPID; echo in sub; exit 5 ); echo st=$?; jobs; fg; echo st=$?' 2>&1
bash -c 'set -m; f() { kill -TSTP $BASHPID; echo in f; }; for i in 1 2; do echo $i | f; done; echo out $?; jobs; kill -9 %1' 2>&1
bash -c 'set -m; true | (exit 3) | { false; }; echo ${PIPESTATUS[@]}; f() { return 4; }; f | f; echo ${PIPESTATUS[@]}; x=1; true | x=2; echo x=$x; ( exit 6 ); echo $? ${PIPESTATUS[@]}' 2>&1
bash -c 'set -m; sleep 1 & jobs | cat; { jobs; } | cat; kill %1' 2>&1 | sed 's/[0-9][0-9]*$/N/'
