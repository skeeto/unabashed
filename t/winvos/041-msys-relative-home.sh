# personality: native
# Hand-checked against Git Bash 5.3.15: an msys shell joins an
# inherited relative HOME or TMP to its working directory as it
# converts it, not to C:/ (the import once ran before the shell knew
# its directory). A native parent passes the values through as is.
mkdir w && cd w
HOME=rel2 TMP='sub\t' C:/msys/usr/bin/bash.exe -c 'echo "$HOME $TMP"'
