== stdout 257
f () 
{ 
    echo $(cat <<A <<B
a
A
b
B
);
    echo $(cat <<E
y
E

OF);
    echo $( ( cat <<E
z
E
 ));
    echo $(cat <<-E
w
E
);
    echo $(case x in x)
        cat <<E
v
E

    ;;
esac);
    echo $(( $(cat <<E
1
E
) + 1 ));
    echo "$(cat <<'E'
u
E
)"
}

== stderr 605
bash: line 4: warning: here-document at line 2 delimited by end-of-file (wanted `A')
bash: line 9: warning: here-document at line 7 delimited by end-of-file (wanted `E')
bash: line 12: warning: here-document at line 10 delimited by end-of-file (wanted `E')
bash: line 15: warning: here-document at line 13 delimited by end-of-file (wanted `E')
bash: line 18: warning: here-document at line 16 delimited by end-of-file (wanted `E')
bash: line 21: warning: here-document at line 19 delimited by end-of-file (wanted `E')
bash: line 24: warning: here-document at line 22 delimited by end-of-file (wanted `E')

== merged 862
bash: line 4: warning: here-document at line 2 delimited by end-of-file (wanted `A')
bash: line 9: warning: here-document at line 7 delimited by end-of-file (wanted `E')
bash: line 12: warning: here-document at line 10 delimited by end-of-file (wanted `E')
bash: line 15: warning: here-document at line 13 delimited by end-of-file (wanted `E')
bash: line 18: warning: here-document at line 16 delimited by end-of-file (wanted `E')
bash: line 21: warning: here-document at line 19 delimited by end-of-file (wanted `E')
bash: line 24: warning: here-document at line 22 delimited by end-of-file (wanted `E')
f () 
{ 
    echo $(cat <<A <<B
a
A
b
B
);
    echo $(cat <<E
y
E

OF);
    echo $( ( cat <<E
z
E
 ));
    echo $(cat <<-E
w
E
);
    echo $(case x in x)
        cat <<E
v
E

    ;;
esac);
    echo $(( $(cat <<E
1
E
) + 1 ));
    echo "$(cat <<'E'
u
E
)"
}

== status 0
