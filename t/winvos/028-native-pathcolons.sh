# personality: native
# Hand-checked baseline: busybox-w32 1.39.0's output for the same lines.
# Assigning PATH in native mode turns its colons into semicolons, all
# but a drive's (right after a letter that starts an element), in an
# assignment, local, export and readonly alike
for p in "C:/a:C:/b;/c:d" ".:x" "a:b" "ab:c" "a:/x:b" "C:x:y" "/x:C:/y" \
         ":a:" "a::b" "1:b" "C:/a;D:b:c" ""; do
    PATH=$p
    echo "[$p] -> [$PATH]"
done
PATH=a:b:c export PATH; echo "$PATH"
f() { local PATH=/q:/r; echo "$PATH"; }; f
( PATH=/s:/t; echo "$PATH" ); echo "$PATH"
readonly PATH=/z:/y; echo "$PATH"
