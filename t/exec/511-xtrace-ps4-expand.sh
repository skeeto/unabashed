PS4='+ $(echo sub) '
set -x
echo one
set +x
PS4='+ ${undef-dflt} '
set -x
echo two
set +x
PS4='+${FOO:=init} '
set -x
echo three "$FOO"
set +x
PS4='+ $((1+1)) '
set -x
echo four
set +x
PS4='+ ${bad x} '
set -x
echo five
set +x
# PS4 is a prompt string: decoded, then expanded as in double quotes,
# so quotes, tildes, <(...) and >(...) are kept as written, also for a
# program started directly as a pipeline stage or in the background
PS4='"q" '\''s'\'' ~ \101 \\ <(:) >(:) * '
set -x
echo six
set +x
PS4='>>( '
set -x
recho seven | >/dev/null
set +x
(set -x; recho eight & { set +x; } 2>/dev/null; wait)
shopt -u promptvars
PS4='$((1+1)) \101 '
set -x
echo nine
set +x
shopt -s promptvars
v=V
x='"$v" \"${v}\" ${u:-"d e"} '\''$v'\'
echo "${x@P}"
echo done
