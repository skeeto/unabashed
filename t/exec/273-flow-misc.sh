# negated and redirected loop control
for i in 1 2; do ! break; done; echo "! break: $?"
for i in 1 2; do ! continue; done; echo "! continue: $?"
for i in 1 2; do break > /tmp/x; echo never; done; echo "redirected break: $i"
for i in 1 2; do continue 2>/dev/null; echo never; done; echo "redirected continue: $i"
for i in 1 2; do v=1 break; echo never; done; echo "assign break: $i v=${v-unset}"
for i in 1 2 3; do true && break; done; echo "and break $i"
for i in 1 2 3; do false || continue; echo never; done; echo "or continue $i"
f() { for i in 1; do ! return 3; done; }
f; echo "! return: $?"
