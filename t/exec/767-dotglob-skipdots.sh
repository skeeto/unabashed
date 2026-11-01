# dotglob and globskipdots decide which dot names wildcards match.
cd /tmp && mkdir dg && cd dg
touch .a .bb c dd
mkdir .h
touch .h/.i .h/j
show() { printf '%s:' "$1"; shift; printf ' %s' "$@"; echo; }
shopt globskipdots dotglob
show '*'      *
show '.*'     .*
show '.?'     .?
show '.??'    .??
show '[.]*'   [.]*
show '.h/*'   .h/*
show '.h/.*'  .h/.*
show '*/*'    */*
shopt -u globskipdots
show 'noskip .*'    .*
show 'noskip .?'    .?
show 'noskip *'     *
show 'noskip .h/.*' .h/.*
shopt -s dotglob
show 'dot noskip *'  *
show 'dot noskip .*' .*
show 'dot noskip ?'  ?
shopt -s globskipdots
show 'dot *'     *
show 'dot .*'    .*
show 'dot */*'   */*
show 'dot ??'    ??
shopt -s globstar
show 'dot **'    **
shopt -u dotglob
show '**'        **
show '.h/**'     .h/**
show '.*/**'     .*/**
