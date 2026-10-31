f() {
    if true; then cat <<E
x
E
    fi
    while false; do cat <<E
y
E
    done
    case x in x) cat <<E
z
E
    esac
    { cat <<E
w
E
    }
    ( cat <<E
v
E
    )
    if true; then cat <<E; fi
u
E
    { cat <<E; }
t
E
    ( cat <<E )
s
E
    case x in x) cat <<E;; esac
r
E
    for i in 1; do cat <<E; done | cat
q
E
}
declare -f
