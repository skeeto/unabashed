# What the shell makes of the Windows environment it was given
declare -p PATH HOME TEMP TMP SYSTEMROOT COMSPEC LANG TZ
declare -p SHELL TMPDIR ORIGINAL_PATH MSYSTEM 2>&1
echo "${TERM-unset}"
env | sort | sed -n '/^\(COMSPEC\|HOME\|LANG\|PATH\|SYSTEMROOT\|TEMP\|TMP\|TZ\)=/p'
