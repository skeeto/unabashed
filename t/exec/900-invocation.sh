# Invocation: long options come first (-name works too), an unknown
# one or a bad letter prints the usage and exits 2, --help and --version
# win over everything after them; -D and --dump-po-strings list $"..."
# strings without running anything; --pretty-print prints a script
bash --badopt 2>&1 | head -3; echo "st ${PIPESTATUS[0]}"
bash -q 2>&1 | sed -n '1p;5p;21,22p'; echo "st ${PIPESTATUS[0]}"
bash +q -c 'echo no' 2>&1 | head -1
bash -c --norc 'echo no' 2>&1 | head -1
bash -x --norc -c 'echo no' 2>&1 | head -1
bash -r +r -c 'echo no' 2>&1 | head -1
bash --rcfile; echo "st $?"
bash -rcfile 2>&1; echo "st $?"
bash -norc -noprofile -c 'echo single dash'
bash -c 'echo $0' --norc
bash --version -q >/dev/null; echo "version $?"
bash --help -c 'exit 3' | sed -n '2,22p'; echo "help $?"
bash -version | wc -l | sed 's/ //g' | grep -v '^0$' >/dev/null && echo "has version"
bash --debug -c 'echo debug'
bash --verbose -c 'echo verbose'
bash -O nosuch +O alsono -c 'echo no'; echo "st $?"

cat > d.sh <<'EOF'
echo $"plain" "$x"
echo $"two
lines" $"q\"b"
echo "${v:-$"inner"}" $(echo $"insub")
echo ran
EOF
bash -D d.sh; echo "st $?"
bash --dump-strings -c 'echo $"from -c"; echo ran'
bash --dump-po-strings d.sh
bash -D -c 'echo $-'

cat > p.sh <<'EOF'
# comment

a=1   # trailing


f() { echo f; }
function g { :; }
if x; then y; fi &
case a in a) b;; esac
for i in 1 2; do echo $i; done | cat
EOF
bash --pretty-print p.sh; echo "st $?"
printf 'echo (\n' > bad.sh
bash --pretty-print bad.sh; echo "st $?"
printf 'echo a\n\n' | bash --pretty-print; echo "st $?"
bash --pretty-print -c 'echo runs'

# BASH_ARGV0 from the environment is $0
BASH_ARGV0=zz bash -c 'echo $0 $BASH_ARGV0'

# The last of -i and +i wins
bash +i -c 'echo "+i $-"'
bash -i +i -c 'echo "-i +i $-"'
