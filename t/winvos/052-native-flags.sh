# personality: native
# Hand-checked against w64devkit's busybox sh: busybox-w32's
# undocumented startup flags. -d DIR changes directory after the login
# profile, so the profile still starts where the shell did, quietly
# not when it names no directory (relative to the shell's own working
# directory, never CDPATH); -t TITLE
# sets the console's title; -s with -c runs the command, then reads
# standard input, with $0 and the positional parameters from after the
# command. An interactive native shell's HISTFILE defaults to busybox's
# ~/.ash_history, and -c's command never goes in the history.
mkdir -p C:/elsewhere sub
echo 'echo profile "$PWD"' >"$HOME/.profile"
sh -d C:/elsewhere -c 'echo "$PWD"'
sh -l -d C:/elsewhere -c 'echo cmd "$PWD"'
sh -d C:/missing -c 'echo still "$PWD"'
CDPATH=C:/ sh -d sub -c 'echo "$PWD"'
echo 'echo stdin "$0" "$1"; echo "$-"' | sh -sc 'echo cmd "$0" "$1"' zero one
echo 'echo never' | sh -sc 'exit 4'; echo "status $?"
printf 'false\n' | sh -s -c 'echo first'; echo "status $?"
sh -t 'a title' -c title
sh -d; echo "status $?"
sh +d C:/ -c : 2>/dev/null; echo "status $?"
(unset HISTFILE; sh -i -c 'echo "$HISTFILE"; history')
