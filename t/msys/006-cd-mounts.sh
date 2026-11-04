# vos
# The mount table: /usr/bin and /bin are Git's usr/bin, /etc is Git's
# etc, /tmp is TEMP, / is the install root
for d in / /usr /usr/bin /bin /etc /tmp /c /dev /proc; do
    (cd "$d" && echo "$d: $(pwd) $(pwd -P) $(pwd -W)")
done
cd /usr/bin/.. && pwd && pwd -W
cd /bin/.. && pwd && pwd -W
cd /tmp/.. && pwd && pwd -W
