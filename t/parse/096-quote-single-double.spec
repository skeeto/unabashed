== stdout 161
f () 
{ 
    echo 'single $x `y`' "double $x `y`" '' "";
    echo 'it'\''s' "a\"b" "a\$b" "a\\b" "a\`b" "a\b";
    echo a'b'"c"d;
    echo "  spaced  " '	tab'
}

== stderr 0

== merged 161
f () 
{ 
    echo 'single $x `y`' "double $x `y`" '' "";
    echo 'it'\''s' "a\"b" "a\$b" "a\\b" "a\`b" "a\b";
    echo a'b'"c"d;
    echo "  spaced  " '	tab'
}

== status 0
