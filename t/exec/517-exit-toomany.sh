(exit 5 6; echo after-sub); echo "sub st=$?"
(shift 1 2; echo after-shift); echo "shift st=$?"
(break 1 2; echo after-break); echo "break st=$?"
(for i in 1; do continue 1 2; done; echo after-cont); echo "cont st=$?"
(eval 'exit 7 8'; echo after-eval); echo "eval st=$?"
x=$(exit 4 5; echo inner); echo "cs st=$? x=$x"
exit 5 6
echo notreached
