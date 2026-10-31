f() {
    echo `cmd` `cmd arg | other` "`cmd`" x`a`y
    echo `echo \`inner\`` `echo \$x` `echo \\` `echo "q"`
    echo `
        multi
    `
}
declare -f
