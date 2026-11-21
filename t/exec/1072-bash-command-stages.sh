# BASH_COMMAND, as bash prints each simple command it forks for before
# forking, is left at the last simple stage of a pipeline, foreground or
# &, with its redirections: a compound stage does not change it. The
# DEBUG action runs for an & pipeline's last stage even when it has
# redirections. In an interactive shell, an action abandoned by SIGINT
# leaves BASH_TRAPSIG bound, and BASH_COMMAND goes on as before.
x='trap "echo \"exit [\$BASH_COMMAND]\"" EXIT; true;'
bash -c "$x"' echo a | cat >/dev/null'
bash -c "$x"' echo a | { cat; } >/dev/null'
bash -c "$x"' { echo a; } | cat >/dev/null'
bash -c "$x"' echo a | cat | (cat) >/dev/null'
bash -c "$x"' ! echo a | cat >/dev/null || :'
bash -c "$x"' [[ x ]] | cat'
bash -c "$x"' f() { cat; }; echo a | f >/dev/null'
bash -c "$x"' g() { echo a | cat >/dev/null; }; g'
bash -c "$x"' echo a | cat >/dev/null &'
bash -c "$x"' echo a >/dev/null &'
bash -c "$x"' { echo a; } >/dev/null &'
bash -c "$x"' set -T; echo a | cat >/dev/null &'
bash -c "$x"' shopt -s lastpipe; echo a | read v'
bash -c 'trap "echo \"usr1 [\$BASH_COMMAND]\"" USR1; true; { sleep 0.2; kill -USR1 $$; } | cat | wc -l | tr -d " "'
echo "-- DEBUG"
bash -c 'trap "echo \"dbg [\$BASH_COMMAND]\"" DEBUG; echo a | cat >/dev/null & wait'
bash -c 'trap "echo \"dbg [\$BASH_COMMAND]\"" DEBUG; echo a | cat 2>/dev/null | cat >/dev/null & wait'
echo "-- interactive"
bash --norc --noediting -i 2>/dev/null <<'EOF'
trap 'echo in; true; kill -INT $$; echo no' USR1
kill -USR1 $$; echo no
echo "sig=[${BASH_TRAPSIG-unset}]"
trap 'echo "usr2 [$BASH_COMMAND]"' USR2
kill -USR2 $$
EOF
echo done
