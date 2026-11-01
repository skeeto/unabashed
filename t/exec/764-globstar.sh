# globstar: ** matches every name below a directory, **/ only
# directories, and runs of ** count once.
shopt -s globstar
cd /tmp && mkdir gs && cd gs
mkdir -p a/b/c d/e
touch a/x a/b/y a/b/c/z d/e/w top
show() { printf '%s:' "$1"; shift; printf ' %s' "$@"; echo; }
show '**'        **
show '**/'       **/
show '**/*'      **/*
show '**/y'      **/y
show 'a/**'      a/**
show 'a/**/'     a/**/
show 'a/**/z'    a/**/z
show '**/b/**'   **/b/**
show '**/**'     **/**
show '**/**/z'   **/**/z
show 'a/**/**'   a/**/**
show 'x**'       x**
show '**x'       **x
show 'a**/y'     a**/y
show '*/**'      */**
show '**/c'      **/c
show '**/c/'     **/c/
shopt -u globstar
show 'off **'    **
show 'off **/*'  **/*
