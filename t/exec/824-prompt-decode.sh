# ${x@P}: backslash escapes, quoting for the expansion that follows
exec 2>&1
cd /tmp
HOST=host SHELL_LEVEL=2 HOME=$PWD
p='\[\]${HOST}($SHELL_LEVEL)[\v]\$ '; printf '<%s>\n' "${p@P}"
p='\[\]\W\$\040'; printf '<%s>\n' "${p@P}"
p='\[\001\][\j]\w\$\040'; printf '<%s>\n' "${p@P}" | od -c | sed 's/  */ /g'
p='\[\a\][\j:\!]\w\\$\040'
set -o emacs
printf '<%s>\n' "${p@P}" | od -c | sed 's/  */ /g'
p='a\nb'; printf '<%s>\n' "${p@P}" | od -c | sed 's/  */ /g'
set +o emacs
printf '<%s>\n' "${p@P}" | od -c | sed 's/  */ /g'
cd /
p='\W \$\040'; printf '<%s>\n' "${p@P}"
p='\w|\#|\x|\D|\'; printf '<%s>\n' "${p@P}"
mkdir -p '/tmp/d$x'; cd '/tmp/d$x'; x=EXP
p='\W'; printf '<%s>\n' "${p@P}"
shopt -u promptvars; printf '<%s>\n' "${p@P}"
p='\$'; printf '<%s>\n' "${p@P}"
shopt -s promptvars
set -o posix
p='!\n\W !! \$\040'; printf '<%s>\n' "${p@P}"
cd /; rmdir '/tmp/d$x'
