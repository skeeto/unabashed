# As 026-con-handoff on a pipe that is the shell's standard input, where
# a pump stays blocked in its read after its program is done only
# without CancelSynchronousIo: XP, or under t/win32/run -x
# (UNABASHED_XPIO).
"$U" -c 'sleep 2; echo l1; sleep 2; echo l2' |
"$U" -c 'read -t 1 x; echo "st=$?"; head -n 1; head -n 1; echo end'
