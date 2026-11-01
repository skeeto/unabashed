# Leading dots must be matched explicitly; . and .. are never matched
# (globskipdots); dotglob lets * match hidden names.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
cd /tmp
: > .hid; : > .h2; : > vis; : > ..x
show *
show .*
show .h*
show ?hid
show [.]hid
show *hid
show .?
show ..*
show .[!.]*
shopt -s dotglob
show *
show ?hid
show .*
show *hid
shopt -u globskipdots
show .*
shopt -u dotglob
show .*
show *
