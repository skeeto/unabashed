# pattern features supported by the matcher
for w in a b x z ab abc 'a.c' A '-' '!'; do
    case $w in
        [ab]) r='[ab]';;
        [!a-y]) r='[!a-y]';;
        ?b) r='?b';;
        a*c) r='a*c';;
        [A-Z]) r='[A-Z]';;
        *) r=default;;
    esac
    echo "$w -> $r"
done
case 'a\b' in 'a\b') echo "literal backslash";; esac
case 'a\b' in a\b) echo "escaped backslash";; esac
case '[' in '[') echo "lone bracket quoted";; esac
case '[' in \[) echo "lone bracket escaped";; esac
case '*a' in \**) echo "escaped star prefix";; esac
case '?' in [?]) echo "bracket question";; esac
