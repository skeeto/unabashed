(set -e; (false); echo a); echo "st=$?"
(set -e; (false; echo inner); echo b); echo "st=$?"
(set -e; (false) || echo rescued; echo c); echo "st=$?"
(set -e; if (false; echo cond-inner); then echo yes; fi; echo d); echo "st=$?"
(set -e; ( (exit 7) ); echo e); echo "st=$?"
(set -e; (set +e; false; echo plus-e); echo f); echo "st=$?"
(set -e; (echo "sub: $-" | { read x; case $x in *e*) echo has-e;; *) echo no-e;; esac; }); echo g)
