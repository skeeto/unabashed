# nocasematch makes case patterns ignore case
shopt -s nocasematch
case ABC in abc) echo "folded";; *) echo "not folded";; esac
case aBc in A?C) echo "folded glob";; esac
case X in [a-z]) echo "folded range";; *) echo "range not folded";; esac
shopt -u nocasematch
case ABC in abc) echo "folded";; *) echo "case sensitive again";; esac
