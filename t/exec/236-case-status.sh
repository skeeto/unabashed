# exit status of case
case x in x) false;; esac
echo "body false: $?"
false
case x in y) echo no;; esac
echo "no match after false: $?"
false
case x in x) ;; esac
echo "empty body after false: $?"
false
case x in x) ;& y) ;; esac
echo "empty fallthrough: $?"
case x in x) (exit 3);& y) ;; esac
echo "fallthrough into empty: $?"
case x in x) (exit 3);;& y) ;; esac
echo "testnext no more: $?"
case $(exit 4) in '') echo "word cmdsub status $?";; esac
case x in $(exit 5)x) echo "pattern cmdsub $?";; esac
false
case x in esac
echo "no items after false: $?"
false
case x in x) echo "body sees previous: $?";; esac
(exit 3)
case x in y) ;; x) echo "second item sees: $?";; esac
