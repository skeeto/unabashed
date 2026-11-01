# compound commands used as if and while conditions
if for i in 1 2; do [ $i = 2 ]; done; then echo "for cond true"; fi
if for i in 1 2; do [ $i = 1 ]; done; then :; else echo "for cond false"; fi
if while false; do :; done; then echo "while cond (never ran) true"; fi
if case a in b) ;; esac; then echo "case no match true"; fi
if case a in a) false;; esac; then :; else echo "case body false"; fi
if (exit 0); then echo "subshell true"; fi
if { true; false; }; then :; else echo "group false"; fi
if if false; then :; fi; then echo "nested if true"; fi
if [[ -n x ]] && (( 2 > 1 )); then echo "cond and arith"; fi
if f() { :; }; then echo "funcdef true"; fi
if x=5; then echo "assignment true $x"; fi
if x=$(exit 3); then :; else echo "assignment cmdsub $?"; fi
i=0
while for j in 1; do [ $i -lt 2 ]; done; do echo "for in while $i"; i=$((i+1)); done
