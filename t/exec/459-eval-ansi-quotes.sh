# eval with ANSI-C quotes, backslashes and special characters
eval "echo \$'a\tb'" | { IFS= read -r l; echo "[$l]"; }
eval 'echo $'"'"'x\ny'"'"
eval 'echo a\ b'
eval "echo 'single \ backslash'"
eval 'echo "dq \ \" \$"'
eval echo '\*'
eval 'echo ~'
eval 'echo #comment'
eval 'echo a # trailing'
