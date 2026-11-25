# A script read from standard input that the shell cannot read (fd 0
# open only for writing, or a directory) ends the shell with 128 and
# "script file read error", its partial line not run; an EXIT trap
# returns $? instead. A closed fd 0 is the end of input.
cd "$(mktemp -d)"
bash <<<'echo hi' 0>&1; echo "status $?"
bash -s a b 0>f; echo "status $?"
printf 'echo a; exec 0>f; echo b\necho c\n' | bash; echo "status $?"
printf 'echo a\nexec 0>f\necho "b\nc"\necho d\n' | bash; echo "status $?"
printf 'echo a\nexec 0>f\necho b\n' >s; bash <s; echo "status $?"
printf 'trap "echo T \\$?" EXIT\nfalse\nexec 0>f\necho b\n' | bash; echo "status $?"
printf 'echo a\nexec 0<&-\necho b\n' | bash; echo "status $?"
mkdir d; bash <d; echo "status $?"
bash -c 'echo c' 0>f; echo "status $?"
