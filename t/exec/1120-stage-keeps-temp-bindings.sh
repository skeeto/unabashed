# bash's child forked for a simple command (a pipeline stage, a job run
# with &) exits once its builtin or function returns, its temporary
# bindings still in place: LC_ALL is not set back, so no setlocale
# warning cites the old value. A compound stage, a function's own
# command, a ( list ) and lastpipe's last stage unbind as usual.
LC_ALL=x
LC_ALL=y : | cat
echo "a $?"
LC_ALL=z true | cat
LC_ALL=w : | LC_ALL=v :
f() { :; }
LC_ALL=u f | cat
LC_ALL=t : | (:)
LC_ALL=s eval : | cat
LC_ALL=r declare q=1 | cat
LC_ALL=y exit 3 | cat
echo "b ${PIPESTATUS[@]}"
g() { return 4; }
LC_ALL=y g | cat
echo "c ${PIPESTATUS[@]}"
LC_ALL=y eval 'echo "<$LC_ALL>"' | cat
LC_ALL=v : &
wait
(LC_ALL=q :)
echo "$(LC_ALL=p :)"
h() { LC_ALL=o :; }
h | cat
{ LC_ALL=n :; } | cat
shopt -s lastpipe
set +m
echo | LC_ALL=m :
echo end
