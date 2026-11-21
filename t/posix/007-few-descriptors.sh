# With few descriptors (a low ulimit -n), the shell's own pipes, files
# and temporary files cannot all move to 10 and up: they stay where they
# are and work, as bash's do. A pipe once came back as made but with no
# descriptor, and a pipeline waited on it for ever.
( ulimit -n 12; exec bash -c 'echo hi | cat; echo "pipeline $?"' )
( ulimit -n 12; exec bash -c 'echo saved >f; cat f; x=$(echo comsub); echo "$x"' )
( ulimit -n 14; exec bash -c 'cat <<EOF | wc -c
$(printf "%70000d" 0)
EOF' )
