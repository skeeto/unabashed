# nocaseglob matches names without regard to case, in wildcards,
# brackets and ranges, while literal components still need the case.
cd /tmp && mkdir nc && cd nc
touch Alpha beta GAMMA delta.TXT Echo.txt
mkdir Dir
touch Dir/File
show() { printf '%s:' "$1"; shift; printf ' %s' "$@"; echo; }
shopt -s nocaseglob
show 'a*'      a*
show '*TXT'    *TXT
show '[a-c]*'  [a-c]*
show '[[:upper:]]*' [[:upper:]]*
show '[[:lower:]]*' [[:lower:]]*
show '?AMMA'   ?AMMA
show 'dir/*'   dir/*
show 'D*/f*'   D*/f*
show '[!a]*'   [!a]*
shopt -s extglob
show '@(alpha|echo)*' @(alpha|echo)*
shopt -u nocaseglob
show 'off a*'  a*
case ALPHA in alpha) echo case folded ;; *) echo case kept ;; esac
