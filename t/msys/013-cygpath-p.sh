# vos
# cygpath -p converts path lists, ':' on the POSIX side, ';' on Windows
cygpath -wp '/c/a:/usr/bin:.'
cygpath -mp '/c/a:/tmp:/d/b'
cygpath -up 'C:\a;D:\b'
cygpath -up 'C:/a;.;x\y'
cygpath -wp ''
cygpath -up ''
cygpath -wp '/c/a::/c/b'
