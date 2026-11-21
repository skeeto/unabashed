# $_ before any command: the environment's _, exported until a command
# rebinds it, or else the shell's argv[0] (not $0 or the script's
# path). A script without #! starts with the path it was run by, as
# its environment's _, exported.
(exec -a foo bash -c 'echo "[$_]"; declare -p _; declare -p _' zero)
env -u _ bash -c 'echo "[$_]"; declare -p _'
env _=/x/y bash -c 'declare -p _; declare -p _'
env _= bash -c 'echo "[$_]"'
env _=/x/y bash -c 'v=$(declare -p _); echo "$v"; declare -p _'
printf 'echo "[$_] [$0]"\n' >s.sh
env -u _ bash s.sh
env _=/q bash <s.sh
env -u _ bash -c "$(cat s.sh)"
printf 'declare -p _\n' >nosb
chmod +x nosb
bash -c './nosb; :'
bash -c 'export _=7; ./nosb; declare -n _=q; ./nosb'
bash -c 'declare -a _=(1 2); ./nosb; declare -p _'
bash -c 'f() { ./nosb; }; f | cat; ./nosb & wait'
bash -c 'command ./nosb; x=$(./nosb); echo "$x"'
bash -c 'command env | grep -c "^_=.*/env$"; command printenv _ | grep -c "/printenv$"; (exec env) | grep -c "^_=.*/env$"'
rm -f s.sh nosb
