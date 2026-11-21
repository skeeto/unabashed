# A trap action run in the middle of read -t: neither the read's
# deadline nor a second trapped signal fails the action's own program;
# the deadline keeps running through the action, so a read whose time
# is up by its end times out even with input waiting
trap 'sleep 2; echo "trap1: sleep $?"' USR1
(sleep 0.3; kill -USR1 $$) &
read -t 1 x < <(sleep 1.5; echo data1)
echo "read1: $? <$x>"
wait
trap 'sleep 0.5; echo "trap2: sleep $?"' USR1
(sleep 0.3; kill -USR1 $$) &
read -t 3 x < <(sleep 1.5; echo data2)
echo "read2: $? <$x>"
wait
trap 'echo usr2' USR2
trap 'sleep 1; echo "trap3: sleep $?"' USR1
(sleep 0.3; kill -USR1 $$; sleep 0.3; kill -USR2 $$) &
read -t 3 x < <(sleep 2; echo data3)
echo "read3: $? <$x>"
wait
# An action abandoned by an error leaves nothing of the read armed (in
# a script, whose eval the error abandons, not -c text)
cat > abort.sh <<'EOF'
trap 'declare -i v; v=1+' USR1
(sleep 0.3; kill -USR1 $$) &
eval 'read -t 1 x < <(sleep 3); echo "read4: $?"' 2>/dev/null
echo "eval4: $?"
sleep 1.5
echo "sleep4: $?"
wait
EOF
bash ./abort.sh
