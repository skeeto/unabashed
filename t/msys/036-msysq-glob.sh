# Wildcards and @file reaching an MSYS2 child stay literal even when
# files would match
: >a.txt
: >b.txt
echo x >resp
/usr/bin/printf '[%s]\n' '*.txt' '?.txt' '[ab].txt' @resp '@resp' ~ '~'
/usr/bin/echo *.txt '*.txt'
