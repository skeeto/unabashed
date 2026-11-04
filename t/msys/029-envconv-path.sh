# vos
# PATH is imported as a ':' list and exported back as a ';' list
echo "$PATH"
ARGV_SHOW=PATH argv
a=$(command -v argv)
PATH=/usr/bin:/c/Windows:.:/tmp ARGV_SHOW=PATH "$a"
(PATH=$PATH:/c/x::; ARGV_SHOW=PATH "$(type -P argv)")
(export ORIGINAL_PATH=/c/a:/c/b; ARGV_SHOW=ORIGINAL_PATH argv)
