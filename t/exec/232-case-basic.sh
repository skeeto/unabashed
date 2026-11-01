# case with several patterns per item, leading parens, defaults
for w in apple banana cherry date ''; do
    case $w in
        apple|banana) echo "$w: a or b";;
        (cherry) echo "$w: c";;
        '') echo "empty";;
        *) echo "$w: default";;
    esac
done
case x in y) echo no;; esac
echo "no match: $?"
case foo in f*) echo star;; foo) echo exact;; esac
case foo in
    bar) echo bar
        ;;
    foo) echo multi
         echo line
         ;;
esac
case z in z) echo "no final semis"
esac
case z in (z) echo "paren no semis"; esac
case q in esac
echo "no items: $?"
