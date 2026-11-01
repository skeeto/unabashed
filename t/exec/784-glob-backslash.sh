# Backslashes in patterns: quoting a glob character, escaping an
# ordinary one, trailing, and backslashes that come from expansions.
cd /tmp && mkdir bk && cd bk
touch 'a*b' 'a\b' ab 'a?' aXb
show() { printf '%s:' "$1"; shift; printf ' %s' "$@"; echo; }
show 'a\*b'     a\*b
show 'a\*'      a\**
show 'a\?'      a\?
show 'a\b'     a\b
show 'a\b'      a\b
v='a\*'; show '$v'      $v
v='a\b'; show '$v'      $v
v='a\b*'; show '$v*'    $v*
v='a\*'; show '$v w'   $v
v='\a*';  show '$v a'   $v
v='a*\';  show 'trailing' $v
case 'a*b' in a\*b) echo case escaped ;; esac
case 'a\b' in 'a\b') echo case quoted ;; esac
p='a\*b'; case 'a*b' in $p) echo case var ;; esac
p='a\b'; case 'ab' in $p) echo case var escape ;; esac
