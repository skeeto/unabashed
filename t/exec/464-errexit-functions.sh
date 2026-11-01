f() { echo f1; false; echo f2; }
(set -e; f; echo a); echo "st=$?"
(set -e; if f; then echo yes; else echo no; fi; echo b); echo "st=$?"
(set -e; f || echo rescued; echo c); echo "st=$?"
(set -e; f && echo and; echo d); echo "st=$?"
(set -e; ! f; echo "e st=$?"); echo "st=$?"
g() { set -e; false; echo g2; }
(g; echo h); echo "st=$?"
k() { false; }
(set -e; while k; do echo no; done; echo i); echo "st=$?"
r() { return 3; }
(set -e; r; echo j); echo "st=$?"
(set -e; r || true; echo k); echo "st=$?"
