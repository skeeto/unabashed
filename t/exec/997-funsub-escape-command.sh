# After ${ cmd; } BASH_COMMAND is the command that ran it again, whether
# the substitution finished or an error abandoned it, and the shell can
# keep running commands either way
cat > s.sh <<'EOF'
declare -i n
: ${ n=1+; }
for w in "$(echo a b c d e f g h; echo more text)" "$BASH_COMMAND"; do echo "w=[$w]"; done
: ${ true; }
for w in "$BASH_COMMAND"; do echo "w=[$w]"; done
x=${ echo inner; } && echo "$x [$BASH_COMMAND]"
EOF
bash s.sh; echo "st $?"
