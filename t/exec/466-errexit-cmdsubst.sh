(set -e; x=$(false; echo inner); echo "a x=$x"); echo "st=$?"
(set -e; x=$(false); echo b); echo "st=$?"
(set -e; echo "c $(false)"; echo c2); echo "st=$?"
(set -e; x=$(true) y=$(false); echo d); echo "st=$?"
(set -e; x=$(false) y=$(true); echo e); echo "st=$?"
(set -e; f() { local v=$(false); echo "local st=$?"; }; f; echo f); echo "st=$?"
(set -e; x=$(set -e; false; echo inner); echo "g x=$x"); echo "st=$?"
(set -e; shopt -s inherit_errexit; x=$(false; echo inner); echo "h x=$x"); echo "st=$?"
(set -e; shopt -s inherit_errexit; echo "i $(false; echo inner)"; echo i2); echo "st=$?"
(set -e; echo "flags: $(echo $-)" | { read a b; case $b in *e*) echo has-e;; *) echo no-e;; esac; })
(set -e; shopt -s inherit_errexit; echo "flags: $(echo $-)" | { read a b; case $b in *e*) echo has-e;; *) echo no-e;; esac; })
