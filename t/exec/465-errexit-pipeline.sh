(set -e; false | true; echo a); echo "st=$?"
(set -e; true | false; echo b); echo "st=$?"
(set -e; set -o pipefail; false | true; echo c); echo "st=$?"
(set -e; set -o pipefail; (exit 3) | (exit 4) | true; echo d); echo "st=$?"
(set -e; ! true | false; echo "e st=$?"); echo "st=$?"
(set -e; echo x | false || echo rescued; echo f); echo "st=$?"
(set -o pipefail; (exit 2) | true; echo "pf st=$?")
(set -o pipefail; true | (exit 2) | true; echo "pf2 st=$?")
(set -o pipefail; (exit 2) | (exit 3) | true; echo "pf3 st=$?")
(false | true; echo "nopf st=$?")
