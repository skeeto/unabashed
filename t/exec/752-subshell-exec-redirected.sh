# exec in a subshell lasts until the subshell ends, even made inside a
# command with redirections of its own, unless that command redirects
# the same descriptor
( { exec 4>&1; } 2>/dev/null; echo group >&4; echo "st=$?" )
( f() { exec 4>&1; }; f >/dev/null; echo func >&4; echo "st=$?" )
( for i in 1; do exec 4>&1; break; done 2>/dev/null; echo loop >&4; echo "st=$?" )
( { exec 4>&1; } 4>/dev/null; echo same >&4; echo "st=$?" )
( x="${ exec 4>&1; echo in; }"; echo funsub >&4; echo "st=$? x=[$x]" )
echo outside >&4; echo "st=$?"
x=$( { exec 4>&1; } 2>/dev/null; echo comsub >&4 ); echo "x=[$x]"
( exec 3>&1; ( exec 3>&-; echo inner >&3 ); echo outer >&3; echo "st=$?" )
printf 'exec 4>&1\necho script\n' >noshebang
chmod +x noshebang
./noshebang; echo after >&4; echo "st=$?"
