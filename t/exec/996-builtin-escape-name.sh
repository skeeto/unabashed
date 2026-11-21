# A builtin abandoned by an error stops being the current builtin: later
# diagnostics outside any builtin carry no builtin's name, and its
# arguments are not reused
cat > s.sh <<'EOF'
declare -i n
declare n=1+
echo "$(echo aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa)" >/dev/null
declare -n r
r='a b'
for i in 1; do r='c d'; done
export n=1+
r='e f'
echo end
EOF
bash s.sh; echo "st $?"
