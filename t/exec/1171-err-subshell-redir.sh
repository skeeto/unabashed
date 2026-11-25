# ERR for compound commands whose own redirections fail, and for a
# ( list ): bash's child for a forked one exits at once, without ERR,
# and a ( list )'s sole command runs in its child's place
set -E; trap 'echo err' ERR
true | { true; } <&9; echo "s=$?"
true | ( true ) <&9; echo "s=$?"
true | while :; do break; done <&9; echo "s=$?"
true | [[ 1 ]] <&9; echo "s=$?"
{ true; } <&9 & wait $!; echo "s=$?"
( { true; } <&9 ); echo "s=$?"
echo ---
( cat <nx ); echo "s=$?"
( ( : ) <nx ); echo "s=$?"
( ( false ) ); echo "s=$?"
( : ) | ( cat nx ); echo "s=$?"
( { cat nx; } ); echo "s=$?"
( x=1 <nx ); echo "s=$?"
echo ---
# the parent's ERR runs once the ( list )'s redirections are undone
( false ) >f1; echo "s=$? [$(cat f1)]"
( local ) &>>f2; echo "s=$?"; cat f2
( exit 3 ) 2>f3 >&2; echo "s=$? [$(cat f3)]"
( false ) >f4 || echo no
