# Under noacl, -x comes from the name (.exe .com .lnk) or the first
# bytes (#!, MZ, :\n), never from chmod
printf '#!/bin/sh\necho a\n' >shebang
printf 'MZ' >mz
printf ':\necho c\n' >colon
printf 'echo d\n' >plain
printf '#!/bin/sh\n' >s.bat
printf 'echo e\n' >e.cmd
printf 'print(1)\n' >p.py
: >empty
: >x.exe
: >y.com
printf ' #!/bin/sh\n' >spaced
printf '#!' >bang
chmod +x plain
for f in shebang mz colon plain s.bat e.cmd p.py empty x.exe y.com spaced bang; do
    [ -x "$f" ] && echo "$f x" || echo "$f -"
done
