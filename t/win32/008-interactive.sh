# shell: -i
# conrun: sleep:500 'type:PS1="$ "\n' 'type:f() { : >ready; while :; do echo x; done; }; f | while read l; do :; done\n' await:ready ctrl-c sleep:500 'type:echo status=$?\n' 'type:g() { : >ready2; while :; do echo x; done; }; trap "echo trapped" INT; g | "$U" -c "cat >/dev/null"; echo "after $?"\n' await:ready2 ctrl-c sleep:500 'type:exit 4\n'
# An interactive shell returns to its prompt when Ctrl-C ends the
# command, with status 130, or runs the trap and goes on. Its standard
# input is the console.
