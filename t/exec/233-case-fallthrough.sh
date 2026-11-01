# ;& falls into the next body, ;;& tests the next patterns
case a in
    a) echo 1;&
    b) echo 2;&
    c) echo 3;;
    d) echo 4;;
esac
echo --
case a in
    a) echo 1;;&
    b) echo 2;;&
    a|c) echo 3;;&
    *) echo 4;;
    a) echo 5;;
esac
echo --
case a in
    a) echo 1;&
    b) ;&
    c) echo 3;&
esac
echo --
case a in
    a) ;;&
    x) echo no;;&
esac
echo "testnext no match: $?"
case a in
    a) echo 1;;&
    *) echo 2;&
    nomatch) echo 3;;&
    a) echo 4
esac
case b in a) echo no;& b) echo fall-from-b;& esac
