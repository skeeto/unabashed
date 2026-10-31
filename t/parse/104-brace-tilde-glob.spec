== stdout 185
f () 
{ 
    echo {a,b,c} pre{1..3}post {a..z..2} {x,{y,z}} {} {a} \{a,b\};
    echo ~ ~/x ~user ~+ ~- ~1 a~b x=~/p PATH=~:~/bin;
    echo * ? [abc] [!a] [^a] [[:alpha:]] *.[ch] **/x
}

== stderr 0

== merged 185
f () 
{ 
    echo {a,b,c} pre{1..3}post {a..z..2} {x,{y,z}} {} {a} \{a,b\};
    echo ~ ~/x ~user ~+ ~- ~1 a~b x=~/p PATH=~:~/bin;
    echo * ? [abc] [!a] [^a] [[:alpha:]] *.[ch] **/x
}

== status 0
