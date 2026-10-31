== stdout 149
f () 
{ 
    echo `cmd` `cmd arg | other` "`cmd`" x`a`y;
    echo `echo \`inner\`` `echo \$x` `echo \\` `echo "q"`;
    echo `
        multi
    `
}

== stderr 0

== merged 149
f () 
{ 
    echo `cmd` `cmd arg | other` "`cmd`" x`a`y;
    echo `echo \`inner\`` `echo \$x` `echo \\` `echo "q"`;
    echo `
        multi
    `
}

== status 0
