# eval of runtime errors and a nonexistent builtin option
eval 'unset_cmd_xyz' 2>&1
echo "st=$?"
eval -- 'echo dashdash'
eval -x 2>&1
echo "opt st=$?"
eval 'readonly ro=1; ro=2' 2>&1
echo "ro st=$?"
eval 'echo ${x:?no x}' 2>&1
echo "after ?: $?"
eval '$(exit 7)'
echo "empty cmd with cmdsub: $?"
