# Standard output and error as one handle value, as MSDN's redirection
# sample passes them (tools/stdrun): each descriptor closes on its own,
# and the other still writes.
S=$BIN/stdrun.exe
"$S" 0 1 1 -- "$U" -c 'exec 2>/dev/null; echo hi; echo gone >&2'
echo "status $?"
"$S" 0 1 1 -- "$U" -c 'exec 1>/dev/null; echo err >&2; echo gone'
echo "status $?"
"$S" 0 1 1 -- "$U" -c 'exec 2>&-; echo out; cat </dev/null; echo "cat $?"'
echo "status $?"
"$S" 0 1 1 -- "$U" -c '{ echo a; echo b >&2; } 2>/dev/null; echo c >&2'
echo "status $?"
