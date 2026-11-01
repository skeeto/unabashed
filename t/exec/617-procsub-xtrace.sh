# A procsub traces as a nested subshell, after the command that uses it
# (it races with that command, so this spec is exempt from chaos runs)
# nochaos
set -x
cat <(echo traced)
read -r v < <(echo "$((1+2))")
set +x
