== stdout 163
f () 
{ 
    if cat <<EOF
cond
EOF
    then
        cat <<EOF
then
EOF

    fi
    case $1 in 
        a)
            cat <<EOF
in case
EOF

        ;;
    esac
}

== stderr 0

== merged 163
f () 
{ 
    if cat <<EOF
cond
EOF
    then
        cat <<EOF
then
EOF

    fi
    case $1 in 
        a)
            cat <<EOF
in case
EOF

        ;;
    esac
}

== status 0
