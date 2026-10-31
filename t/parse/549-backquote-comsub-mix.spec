== stdout 277
f () 
{ 
    echo `echo $(echo a)`;
    echo $(echo `echo b`);
    echo `echo \`echo c\` $(echo d)`;
    echo $(echo `echo \`echo e\``);
    echo "`echo "$(echo "f")"`";
    echo `echo $(case x in x) echo g;; esac)`;
    echo $(echo `echo h # )`);
    echo `echo \$(echo i)`
}

== stderr 0

== merged 277
f () 
{ 
    echo `echo $(echo a)`;
    echo $(echo `echo b`);
    echo `echo \`echo c\` $(echo d)`;
    echo $(echo `echo \`echo e\``);
    echo "`echo "$(echo "f")"`";
    echo `echo $(case x in x) echo g;; esac)`;
    echo $(echo `echo h # )`);
    echo `echo \$(echo i)`
}

== status 0
