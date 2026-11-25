# bash prints a command substitution's case with a here-document
# without the `;' after esac, so that text fails to parse again when
# the substitution is expanded. Directly in a word, that abandons the
# command (status 1). One nested in another fails while the outer one
# is scanned, in bash's parse_comsub, which ends a non-interactive
# shell with the syntax error's status: 2, or 127 from -c; a ( )
# subshell ends itself with 2.
cat >d.sh <<'EOF'
echo "[$(case a in *) : <<E;; esac; echo no
E
)]"; echo same line
EOF
cat >n.sh <<'EOF'
echo "$(: "$(case a in *) : <<E;; esac; echo no
E
)")"; echo same line
EOF
cat >u.sh <<'EOF'
x=$(: $(case a in *) : <<E;; esac; echo no
E
)); echo same line
EOF
while IFS= read -r c; do
    echo "-- $c"
    bash -c "trap 'echo exit \$?' EXIT; $c
echo \"after \$?\"" 2>&1
done <<'EOF'
eval "$(cat d.sh)"
eval "$(cat n.sh)"
eval "$(cat u.sh)"
. ./n.sh
( . ./n.sh; echo in ); echo "sub $?"
f() { . ./n.sh; echo in; }; f
EOF
bash d.sh 2>&1; echo "script $?"
bash n.sh 2>&1; echo "script $?"
rm -f d.sh n.sh u.sh
