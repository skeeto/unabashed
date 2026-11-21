# A background job copies BASH_COMMAND's text whole, both when the
# command's text is already set and while a command is being run
trap 'for x in "$BASH_COMMAND"; do printf "child: %q\n" "$x"; done & wait' EXIT
long=$(printf '%09000d' 0)
f() { printf '%s %s\n' "${#BASH_COMMAND}" "${BASH_COMMAND:0:12}"; }
f "$long" & wait
: "$long" "$long" && { f; } & wait
echo hello world
