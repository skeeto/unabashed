# A script without #! starts from the default set and shopt options and
# no aliases, as bash's reinitialized subshell, keeping only what bash
# keeps; the caller's options are back afterward
mkdir -p d
printf 'echo "$-"; shopt -p | grep -v login_shell; set -o | grep -e posix -e pipefail; alias\n' > d/opts
chmod +x d/opts
d/opts > fresh.txt
set -u -f -o pipefail -o posix
shopt -s dotglob extglob nullglob xpg_echo compat44 lastpipe
shopt -u extquote sourcepath
alias ll='ls -l'
d/opts > after.txt
cmp fresh.txt after.txt && echo same
cat after.txt
echo "$-"; shopt -p dotglob extquote xpg_echo; set -o | grep -e posix -e pipefail; alias
