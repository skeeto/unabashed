== stdout 262
[C:/a:C:/b;/c:d] -> [C:/a;C:/b;/c;d]
[.:x] -> [.;x]
[a:b] -> [a:b]
[ab:c] -> [ab;c]
[a:/x:b] -> [a:/x;b]
[C:x:y] -> [C:x;y]
[/x:C:/y] -> [/x;C:/y]
[:a:] -> [;a:]
[a::b] -> [a:;b]
[1:b] -> [1;b]
[C:/a;D:b:c] -> [C:/a;D:b;c]
[] -> []
a:b;c
/q;/r
/s;/t
a:b;c
/z;/y

== stderr 0

== merged 262
[C:/a:C:/b;/c:d] -> [C:/a;C:/b;/c;d]
[.:x] -> [.;x]
[a:b] -> [a:b]
[ab:c] -> [ab;c]
[a:/x:b] -> [a:/x;b]
[C:x:y] -> [C:x;y]
[/x:C:/y] -> [/x;C:/y]
[:a:] -> [;a:]
[a::b] -> [a:;b]
[1:b] -> [1;b]
[C:/a;D:b:c] -> [C:/a;D:b;c]
[] -> []
a:b;c
/q;/r
/s;/t
a:b;c
/z;/y

== status 0
