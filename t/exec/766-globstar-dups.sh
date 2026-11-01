# Patterns with ** in the middle list some names more than once, as
# bash's expansion does.
shopt -s globstar
cd /tmp && mkdir gd && cd gd
mkdir -p a/a/a b/a
show() { printf '%s:' "$1"; shift; printf ' %s' "$@"; echo; }
show '**/a/**'     **/a/**
show '**/a'        **/a
show '**/a/'       **/a/
show '**/a/*'      **/a/*
show 'a/**/a'      a/**/a
show '**/**/a/**'  **/**/a/**
show 'a/**/**/a'   a/**/**/a
show '*/**/a'      */**/a
echo "${#}" ; set -- **/a/**; echo "$#"
