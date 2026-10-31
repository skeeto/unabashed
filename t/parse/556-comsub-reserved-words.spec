== stdout 563
f () 
{ 
    echo $(echo if then fi; for x in do done;
do
    echo done;
done);
    echo $(case in in in)
        echo in
    ;;
esac);
    echo $(if if true; then
    true;
fi; then
    echo then;
fi);
    echo $({ echo }; });
    echo $(! true) $(time echo t) $(time -p ! false);
    echo $(function function () 
{ 
    :
});
    echo $(while while false; do
    :;
done; do
    :;
done);
    echo $(for for in for;
do
    echo for;
done);
    echo $(select do in done;
do
    break;
done);
    echo $(echo { } [[ ]] ! esac);
    echo $(x=fi; if=1; echo $if)
}

== stderr 0

== merged 563
f () 
{ 
    echo $(echo if then fi; for x in do done;
do
    echo done;
done);
    echo $(case in in in)
        echo in
    ;;
esac);
    echo $(if if true; then
    true;
fi; then
    echo then;
fi);
    echo $({ echo }; });
    echo $(! true) $(time echo t) $(time -p ! false);
    echo $(function function () 
{ 
    :
});
    echo $(while while false; do
    :;
done; do
    :;
done);
    echo $(for for in for;
do
    echo for;
done);
    echo $(select do in done;
do
    break;
done);
    echo $(echo { } [[ ]] ! esac);
    echo $(x=fi; if=1; echo $if)
}

== status 0
