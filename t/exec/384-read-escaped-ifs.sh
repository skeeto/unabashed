# escaped IFS characters do not split
read a b <<<'x\ y z'; echo "a=[$a] b=[$b]"
IFS=: read a b <<<'p\:q:r'; echo "a=[$a] b=[$b]"
read -r a b <<<'x\ y z'; echo "a=[$a] b=[$b]"
read a <<<'trail\ '; echo "a=[$a]"
read a b <<<'one two\ '; echo "a=[$a] b=[$b]"
read a b <<<'\ lead rest'; echo "a=[$a] b=[$b]"
IFS=: read a <<<'end\:'; echo "a=[$a]"
