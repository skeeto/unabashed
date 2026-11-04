== stdout 512
/c/windows
c:\windows
c:/windows
/c/windows/system32
c:\windows\system32
c:/windows/system32
/c/windows
/c/a
/tmp
/d/b c
status 1
C:/a
@WORK@/063-cygpath-opts/tmp
D:/b c
C:\a
@WORK@\063-cygpath-opts\tmp
D:\b c
/c/a
/tmp
C:\a;@WORK@\063-cygpath-opts\tmp
C:\Windows
C:/Windows
@WORK@/063-cygpath-opts/sub
-x
@WORK@\063-cygpath-opts
@WORK@/063-cygpath-opts/b
/c/Windows
C:\a
C:\b
status 0
status 1
status 1
/c/a
status 1
no args 1
bad option 1
two forms 1
missing file 1
-W with a name 1
/c/a:/d/b
C:/a;/d/b
C:\a;.

== stderr 204
cygpath: can't convert empty path
cygpath: can't convert empty path
cygpath: can't convert empty path
cygpath: can't convert empty path
cygpath: can't convert empty path
cygpath: can't convert empty path

== merged 716
/c/windows
c:\windows
c:/windows
/c/windows/system32
c:\windows\system32
c:/windows/system32
/c/windows
/c/a
/tmp
/d/b c
cygpath: can't convert empty path
status 1
C:/a
@WORK@/063-cygpath-opts/tmp
D:/b c
cygpath: can't convert empty path
C:\a
@WORK@\063-cygpath-opts\tmp
D:\b c
cygpath: can't convert empty path
/c/a
/tmp
C:\a;@WORK@\063-cygpath-opts\tmp
C:\Windows
C:/Windows
@WORK@/063-cygpath-opts/sub
-x
@WORK@\063-cygpath-opts
@WORK@/063-cygpath-opts/b
/c/Windows
C:\a
C:\b
status 0
cygpath: can't convert empty path
status 1
cygpath: can't convert empty path
status 1
cygpath: can't convert empty path
/c/a
status 1
no args 1
bad option 1
two forms 1
missing file 1
-W with a name 1
/c/a:/d/b
C:/a;/d/b
C:\a;.

== status 0
