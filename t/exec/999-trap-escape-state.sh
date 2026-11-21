# A trap action abandoned by an error leaves the shell as a finished one
# does: the next signal runs it again, however many times, and a DEBUG
# trap that failed still runs before the next command
cat > s.sh <<'EOF'
trap 'declare -i v; v=1+' USR1
i=0
while [ $i -lt 12 ]; do kill -USR1 $$; i=$((i+1)); done
echo survived $i
trap - USR1
trap 'echo debug; declare -i v; v=1+' DEBUG
:
trap 'echo DEBUG ran' DEBUG
:
trap - DEBUG
echo end
EOF
bash s.sh; echo "st $?"
