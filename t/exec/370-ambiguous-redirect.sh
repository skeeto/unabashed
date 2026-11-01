# targets that do not expand to exactly one word
empty=
multi="a b"
echo x >$empty
echo status $?
echo x >$unset
echo status $?
echo x >$multi
echo status $?
echo x >"$empty"
echo status $?
echo x >"$multi"
echo status $?
read y <$multi
echo status $?
echo x 2>$empty
echo status $?
echo x &>$multi
echo status $?
{ echo grouped; } >$empty
echo status $?
f() { echo f; }
f >${multi}
echo status $?
echo x >$empty"" 
echo status $?
