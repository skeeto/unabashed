== stdout 145
f () 
{ 
    a > "$file";
    a > 'q f';
    a > $dir/x.$$;
    a < ~/in;
    a > ${x:-def};
    a > $(name);
    a 2> /dev/null;
    a > a\ b
}

== stderr 0

== merged 145
f () 
{ 
    a > "$file";
    a > 'q f';
    a > $dir/x.$$;
    a < ~/in;
    a > ${x:-def};
    a > $(name);
    a 2> /dev/null;
    a > a\ b
}

== status 0
