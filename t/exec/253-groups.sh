# { } groups run in the current shell
x=1
{ x=2; echo "in group $x"; }
echo "after group $x"
{ false; }; echo "group false: $?"
{ true; false; true; }; echo "group last: $?"
{ echo a; echo b; } > /tmp/g
while read l; do echo "read $l"; done < /tmp/g
{ read first; read second; } < /tmp/g
echo "$second/$first"
{ { { echo nested; }; }; }
{ echo "stderr" >&2; } 2>&1
{ echo out; echo err >&2; } 2>/dev/null
{
    echo multi
    echo line
}
f() { { return 3; }; echo never; }
f; echo "return in group: $?"
{ echo piped; } | { read v; echo "got $v"; }
