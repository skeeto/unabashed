t() { if eval "$1"; then echo "T: $1"; else echo "F: $1"; fi; }
t '[[ ABC == abc ]]'
shopt -s nocasematch
t '[[ ABC == abc ]]'
t '[[ ABC == a* ]]'
t '[[ ABC != abc ]]'
t '[[ X == [a-z] ]]'
t '[[ ABC < abd ]]'
case XyZ in xyz) echo case-match;; *) echo case-nomatch;; esac
shopt -u nocasematch
t '[[ ABC == a* ]]'
case XyZ in xyz) echo case-match;; *) echo case-nomatch;; esac
