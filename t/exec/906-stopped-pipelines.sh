# Foreground pipelines under set -m: their programs share a process
# group. When the last program stops, the pipeline is a stopped job
# whose text is the whole pipeline; $? is 128 and PIPESTATUS a lone 0,
# as bash. fg continues it and takes the last program's status. A TSTP
# stop breaks the loop. (A stop of an earlier stage alone would hang,
# as in bash, while the last waits for its input.)
bash -c 'set -m; true | bash -c "kill -STOP \$\$"; echo st=$? ${PIPESTATUS[@]}; jobs; fg; echo st=$? ${PIPESTATUS[@]}' 2>&1
bash -c 'set -m; echo hi | bash -c "kill -STOP \$\$; cat"; echo st=$?; jobs; fg; echo st=$?' 2>&1
bash -c 'set -m; f() { :; }; f | bash -c "kill -TSTP \$\$; exit 3"; echo st=$?; jobs; fg; echo st=$?' 2>&1
bash -c 'set -m; for i in 1 2; do true | bash -c "kill -TSTP \$\$"; echo $i; done; jobs; kill -9 %1' 2>&1
bash -c 'set -m; true | false | true; echo ${PIPESTATUS[@]}; echo a | cat | tr a b' 2>&1
