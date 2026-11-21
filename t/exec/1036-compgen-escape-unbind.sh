# A -F function that escapes leaves no COMP_* variables behind, as
# bash's unwind-protect unbinds them however it ends
cat > s.sh <<'EOF'
f() { declare -i n; n=1/0; echo in; }
compgen -F f x 2>/dev/null
echo "st $? [${COMP_LINE-unset}] [${COMP_POINT-unset}] [${COMP_TYPE-unset}]"
declare -p COMP_LINE COMP_POINT COMP_TYPE COMP_KEY COMP_WORDS COMP_CWORD
g() { echo ${y!!}; }
compgen -F g x 2>/dev/null
echo "[${COMP_LINE-unset}] [${COMP_KEY-unset}]"
h() { COMPREPLY=(hx); }
compgen -F h x 2>/dev/null
echo "[${COMP_LINE-unset}] [${COMP_POINT-unset}]"
EOF
bash s.sh; echo "st $?"
