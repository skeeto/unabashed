# Redirection words undergo pathname expansion and must give one word;
# in POSIX mode a non-interactive shell neither globs nor splits them.
# $(< file) expands its file the same way.
cd /tmp && mkdir rg && cd rg
echo content > only.txt
cat < o*.txt
echo "[$(< o*.txt)]"
echo more > other.txt
cat < o*.txt; echo "status $?"
x=$(< o*.txt); echo "status $? [$x]"
f='only.txt other.txt'
cat < $f; echo "split $?"
set -o posix
cat < o*.txt; echo "posix status $?"
x=$(< o*.txt); echo "posix comsub $? [$x]"
cat < $f; echo "posix split $?"
set +o posix
: > 'o*.txt'
echo literal > 'o*.txt'
set -o posix
cat < o*.txt
set +o posix
