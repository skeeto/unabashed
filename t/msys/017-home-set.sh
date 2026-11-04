# vos
# HOME set in the shell in either form: tilde uses it as is, and a
# native child receives it converted back to backslashes
export ARGV_SHOW=HOME
argv
HOME='C:\Windows'
echo ~ ~/x
argv
export HOME=/c/Windows
echo ~
argv
cd ~ && pwd
HOME=C:/Windows/System32
cd ~ && pwd
argv
HOME=
echo ~
