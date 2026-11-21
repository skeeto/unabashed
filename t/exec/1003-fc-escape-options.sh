# fc puts history and verbose back and removes its edit file even when
# an error abandons the command it reruns
mkdir tmp
export TMPDIR=$PWD/tmp
cat > s.sh <<'EOF'
set -o history
declare -i n
echo one
fc -s one='n=1+'
set -o | grep -E '^(history|verbose)'
history 2
printf '%s\n' 'echo edited' 'n=1+' > cmd.txt
FCEDIT='cp cmd.txt' fc -1 2>err.txt
sed 's/^[^:]*: //' err.txt
set -o | grep -E '^(history|verbose)'
ls tmp
history 3
EOF
bash s.sh; echo "st $?"
