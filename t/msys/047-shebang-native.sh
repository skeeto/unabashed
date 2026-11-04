# A shebang naming a native program: its command line shows how the
# script and its arguments reach it
a=$(command -v argv)
printf '#!%s\n' "$a" >s1
printf '#!%s opt\n' "$(cygpath -m "$a")" >s2
printf '#!%s\n' "${a%.exe}" >s3
./s1 x '/c/y'
./s2 'a b'
./s3
