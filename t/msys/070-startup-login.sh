# shell: -l test.sh a
# A login shell: /etc/profile through the mount table, then
# ~/.bash_profile; no chdir
echo "script $0 $1 $x"
shopt login_shell
echo "$PWD $HOME"
