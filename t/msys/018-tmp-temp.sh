# TEMP and TMP become /tmp, which is mounted on the Windows TEMP;
# TMPDIR is not set by the shell
echo "$TEMP $TMP ${TMPDIR-unset}"
(cd /tmp && pwd -W)
cygpath -m /tmp
echo x >"$TMP/f" && cat "$TEMP/f" && cat /tmp/f
ARGV_SHOW='TEMP TMP TMPDIR' argv
export TMPDIR=/tmp
ARGV_SHOW=TMPDIR argv
