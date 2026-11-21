# The redirections in force for the EXIT action after a fatal error are
# what bash's jump to its top level leaves: raised outside eval, source,
# a trap action or ${ cmd; }, none (none undone in a subshell of -c);
# raised inside, all but those the outermost of these made, so that
# from a ${ cmd; } the action writes into its capture, lost; under -c
# the text's own frame undoes all in the shell itself
cases=(
  '{ : ${u?}; } >out'
  'f() { : ${u?}; } >out; f'
  'f() { : ${u?}; }; f >out'
  '{ eval ": \${u?}"; } >out'
  'eval "{ : \${u?}; } >out"'
  'echo ${ : ${u?}; }'
  'x=${ echo a; : ${u?}; }'
  '{ echo ${ : ${u?}; }; } >out'
  'echo ${ { : ${u?}; } >out; }'
  'echo ${ exec >out; : ${u?}; }'
  'eval "echo \${ : \${u?}; }"'
  'echo ${| : ${u?}; }'
  '( trap "echo sub-exit" EXIT; { : ${u?}; } >out ); echo after'
  '( trap "echo sub-exit" EXIT; echo ${ : ${u?}; } ); echo after'
)
for c in "${cases[@]}"; do
  for mode in script -c; do
    rm -f out
    s="trap 'echo in-exit' EXIT; $c"
    if [ $mode = script ]; then
      printf '%s\n' "$s" >s.sh
      bash s.sh >o 2>/dev/null
    else
      bash -c "$s" >o 2>/dev/null
    fi
    printf '%-60s %-6s %s out=[%s] file=[%s]\n' "$c" $mode $? "$(<o)" "$(cat out 2>/dev/null)"
  done
done
