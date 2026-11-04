# vos
# How argv[0] and the program path reach a native child, by each name
argv
argv.exe
argv32 x
a=$(command -v argv)
"$a" y
"$(cygpath -m "$a")" z
"$(cygpath -w "$a")" w
(exec -a fake argv v)
(PATH=; argv) 2>&1; echo "status $?"
