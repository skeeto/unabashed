# transformations on $@ $* and positionals
p() { printf '<%s>' "$@"; echo; }
set -- 'a\tb' "it's" ''
p "${@@Q}"
p "${@@E}"
p "${*@Q}"
IFS=-; p "${*@Q}"; unset IFS
p "${2@Q}" "${3@Q}" "${4@Q}"
p "${@@A}"
p "${@@a}"
