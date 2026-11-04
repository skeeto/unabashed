# Arguments to an MSYS2 child arrive intact: no path conversion, no
# globbing, no @file, whatever the quoting
/usr/bin/printf '[%s]\n' '' 'a b' 'a"b' 'a\b' 'a\\b' 'a\' 'a\"b' '*' '?' \
    '[x]' '~' '~/x' '@file' '{a,b}' "a'b" 'tab	x' /c/x /tmp 'C:\x' '"' \
    '\\server\share' '//c' '(x)' 'é'
