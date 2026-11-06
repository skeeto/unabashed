# A signal to a background job reaches the program bash would have
# exec'd in its place: a lone simple command, or a subshell's last; a
# group runs it as a child, and dies of the signal while it runs on
bash -c 'trap "echo kid TERM" TERM; sleep 2; echo kid end' & sleep 1; kill $!; wait $!; echo "kid $?"
( bash -c 'trap "echo sub TERM" TERM; sleep 2; echo sub end' ) & sleep 1; kill $!; wait $!; echo "sub $?"
{ bash -c 'trap "echo grp TERM" TERM; sleep 2; echo grp end'; } & sleep 1; kill $!; wait $!; echo "grp $?"
sleep 2
