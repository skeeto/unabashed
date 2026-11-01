# conrun: await:ready ctrl-c
# Ctrl-C reaches every stage of a pipeline of shell code; the trap
# runs once its stages died of it, and the script goes on
trap 'echo caught' INT
f() { : >ready; while :; do echo x; done; }
f | while read l; do :; done
echo "after $? ${PIPESTATUS[*]}"
