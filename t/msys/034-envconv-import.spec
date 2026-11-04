== stdout 660
declare -x PATH="/usr/bin:@W64@/bin:/c/WINDOWS/System32:@WORK@/bin"
declare -x HOME="@WORK@/034-envconv-import/home"
declare -x TEMP="/tmp"
declare -x TMP="/tmp"
declare -x SYSTEMROOT="C:\\WINDOWS"
declare -x COMSPEC="C:\\WINDOWS\\System32\\cmd.exe"
declare -x LANG="C"
declare -x TZ="UTC"
declare -- SHELL="/usr/bin/bash"
test.sh: line 3: declare: TMPDIR: not found
test.sh: line 3: declare: ORIGINAL_PATH: not found
test.sh: line 3: declare: MSYSTEM: not found
xterm-256color
COMSPEC=C:\WINDOWS\System32\cmd.exe
HOME=@WORK@/034-envconv-import/home
LANG=C
PATH=/usr/bin:@W64@/bin:/c/WINDOWS/System32:@WORK@/bin
SYSTEMROOT=C:\WINDOWS
TEMP=/tmp
TMP=/tmp
TZ=UTC

== stderr 0

== merged 660
declare -x PATH="/usr/bin:@W64@/bin:/c/WINDOWS/System32:@WORK@/bin"
declare -x HOME="@WORK@/034-envconv-import/home"
declare -x TEMP="/tmp"
declare -x TMP="/tmp"
declare -x SYSTEMROOT="C:\\WINDOWS"
declare -x COMSPEC="C:\\WINDOWS\\System32\\cmd.exe"
declare -x LANG="C"
declare -x TZ="UTC"
declare -- SHELL="/usr/bin/bash"
test.sh: line 3: declare: TMPDIR: not found
test.sh: line 3: declare: ORIGINAL_PATH: not found
test.sh: line 3: declare: MSYSTEM: not found
xterm-256color
COMSPEC=C:\WINDOWS\System32\cmd.exe
HOME=@WORK@/034-envconv-import/home
LANG=C
PATH=/usr/bin:@W64@/bin:/c/WINDOWS/System32:@WORK@/bin
SYSTEMROOT=C:\WINDOWS
TEMP=/tmp
TMP=/tmp
TZ=UTC

== status 0
