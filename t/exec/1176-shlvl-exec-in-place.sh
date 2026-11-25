# bash execs a program without forking first as the last command of
# -c, a comsub, a procsub or a ( ), and as a job's or ( ) stage's
# simple command: SHLVL is one lower for it, as for exec, so a shell
# it starts is at the level of the one it replaces. Not in a pipeline
# stage's child, nor exec in a ( )'s.
echo "top $SHLVL"
bash -c 'printenv SHLVL'
bash -c 'bash -c "echo c2 \$SHLVL"'
bash -c 'bash -c "echo c3 \$SHLVL"; :'
bash -c ':; printenv SHLVL'
bash -c 'false || printenv SHLVL'
bash -c ': && printenv SHLVL >&1'
bash -c '{ printenv SHLVL; }'
bash -c '! printenv SHLVL'
bash -c 'trap : INT; printenv SHLVL'
bash -c 'trap "" INT; printenv SHLVL'
bash -c 'f() { printenv SHLVL; }; f'
bash -c 'eval printenv SHLVL'
bash -c 'command printenv SHLVL'
echo "paren $(printenv SHLVL)" "$(:; printenv SHLVL)" "$(printenv SHLVL; :)"
echo "redir $(printenv SHLVL 2>&1)" "$(f() { printenv SHLVL; }; f)"
cat <(printenv SHLVL 2>&1)
(printenv SHLVL); (printenv SHLVL >&1); (:; printenv SHLVL); ({ printenv SHLVL; })
(: && printenv SHLVL); (trap 'echo t' EXIT; printenv SHLVL); ( (printenv SHLVL) )
printenv SHLVL & wait
(printenv SHLVL) & wait
{ printenv SHLVL; } & wait
echo | printenv SHLVL
echo | (printenv SHLVL)
echo | { printenv SHLVL; }
echo "pipecs $(echo | { echo $(printenv SHLVL); })"
echo "-- exec"
bash -c 'exec printenv SHLVL'
bash -c 'echo | exec printenv SHLVL'
(exec printenv SHLVL)
echo "$(exec printenv SHLVL)" "$( (exec printenv SHLVL) )"
{ exec printenv SHLVL; } & wait
bash -c 'SHLVL=9 exec printenv SHLVL'
bash -c 'exec -c printenv SHLVL || echo none'
echo "-- values"
bash -c 'SHLVL=9 printenv SHLVL'
bash -c 'SHLVL=9 printenv SHLVL; :'
bash -c 'unset SHLVL; printenv SHLVL'
bash -c 'declare +x SHLVL; printenv SHLVL'
bash -c 'SHLVL=0; printenv SHLVL'
bash -c 'SHLVL=" 7 "; printenv SHLVL'
bash -c 'SHLVL=abc; printenv SHLVL'
bash -c 'declare -i SHLVL; SHLVL=3+4; printenv SHLVL'
bash -c 'SHLVL=1001; printenv SHLVL'
bash -c 'readonly SHLVL; printenv SHLVL'
bash -c 'shopt -s execfail; SHLVL=5; declare +x SHLVL; exec ./nonx 2>/dev/null; printenv SHLVL; :'
bash -c 'shopt -s execfail; SHLVL=x; exec ./nonx 2>/dev/null; echo "$SHLVL"'
echo "-- script"
printf 'echo "script $SHLVL"\n' > s; chmod +x s
bash -c ./s
bash -c './s; :'
