# vos
# cygpath -u (the default): Windows to POSIX, mount-aware
cygpath 'C:\Users'
cygpath -u 'C:/Windows'
cygpath -u 'c:/windows/System32'
cygpath -u 'C:'
cygpath -u 'C:\'
cygpath -u 'c:foo'
cygpath -u 'a\b'
cygpath -u 'a/b\c'
cygpath -u "$(pwd -W)"
cygpath -u "$(pwd -W)/test.sh"
cygpath -u "$(cd /usr/bin && pwd -W)"
cygpath -u "$(cd /etc && pwd -W)/fstab"
cygpath -u "$(cd /tmp && pwd -W)"
cygpath -u 'D:\x' 'E:/y'
