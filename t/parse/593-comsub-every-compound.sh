f() {
    x=$(if a; then b; elif c; then d; else e; fi; while f; do g; done; until h; do i; done)
    x=$(for j in k l; do m; done; for ((n=0; n<1; n++)); do o; done; select p in q; do r; done)
    x=$(case s in t) u;; v|w) x;& y) z;;& esac; { aa; bb; }; ( cc; dd ))
    x=$([[ ee && ! ff || ( gg ) ]]; (( hh )); time ii | jj; ! kk |& ll)
    x=$(function mm { nn; }; oo() ( pp ); qq() [[ rr ]]; ss() (( tt )))
    x=$(coproc uu; coproc VV { ww; }; xx & yy && zz || aaa; bbb &)
    x=$(cmd >out 2>&1 <in 3<>rw 4>&- >|clob &>both &>>app <&5 {fd}>x)
    x=$(cmd <<E <<-F <<<here
e
E
	f
	F
)
    x=$(
        if a
        then
            b
        fi
    )
}
declare -f
