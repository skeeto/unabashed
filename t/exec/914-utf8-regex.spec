== stdout 1552
é: alpha !upper lower !punct alnum !space print graph
É: alpha upper !lower !punct alnum !space print graph
ß: alpha !upper lower !punct alnum !space print graph
€: !alpha !upper !lower punct !alnum !space print graph
中: alpha !upper !lower !punct alnum !space print graph
٣: alpha !upper !lower !punct alnum !space print graph
a: alpha !upper lower !punct alnum !space print graph
m é
m ÉTÉ
icase eq
icase bracket
bash: line 21: [[: invalid regular expression `^[à-ê]+': Invalid collation character
icase omega
^\w+$ aéb aéb
^\w+$ é- -
^\w+$ $'x\377y' -
^\w+$ $'\377' -
^\w+$ a\ b -
^\w+$ a b -
^\w+$ a　b -
\W aéb -
\W é- -
\W $'x\377y' -
\W $'\377' -
\W a\ b \ 
\W a b  
\W a　b 　
\<é aéb -
\<é é- é
\<é $'x\377y' -
\<é $'\377' -
\<é a\ b -
\<é a b -
\<é a　b -
a\> aéb -
a\> é- -
a\> $'x\377y' -
a\> $'\377' -
a\> a\ b a
a\> a b a
a\> a　b a
\bé aéb -
\bé é- é
\bé $'x\377y' -
\bé $'\377' -
\bé a\ b -
\bé a b -
\bé a　b -
é\B aéb é
é\B é- -
é\B $'x\377y' -
é\B $'\377' -
é\B a\ b -
é\B a b -
é\B a　b -
\s aéb -
\s é- -
\s $'x\377y' -
\s $'\377' -
\s a\ b \ 
\s a b -
\s a　b 　
é =~ ^[[:lower:]]$: [é]
É =~ ^[[:lower:]]$: [É]
é =~ ^[[:upper:]]$: [é]
ǆ =~ ǅ: [ǆ]
ǅ =~ ǆ: [ǅ]
ı =~ i: [ı]
ı =~ I: [ı]
I =~ ı: [I]
i =~ ı: [i]
ſ =~ s: [ſ]
ß =~ ẞ: no
xÉy =~ x.y: [xÉy]
aÉÉb =~ a(é+)b: [aÉÉb]
€ =~ [^[:alpha:]]: [€]
Straße =~ STRASSE: no
€ =~ [^[:alpha:]]: [€]
a中b =~ a[[:alpha:]]b: [a中b]
a中b =~ a[^[:alpha:]]b: no
x٣ =~ [[:digit:]]: no

== stderr 0

== merged 1552
é: alpha !upper lower !punct alnum !space print graph
É: alpha upper !lower !punct alnum !space print graph
ß: alpha !upper lower !punct alnum !space print graph
€: !alpha !upper !lower punct !alnum !space print graph
中: alpha !upper !lower !punct alnum !space print graph
٣: alpha !upper !lower !punct alnum !space print graph
a: alpha !upper lower !punct alnum !space print graph
m é
m ÉTÉ
icase eq
icase bracket
bash: line 21: [[: invalid regular expression `^[à-ê]+': Invalid collation character
icase omega
^\w+$ aéb aéb
^\w+$ é- -
^\w+$ $'x\377y' -
^\w+$ $'\377' -
^\w+$ a\ b -
^\w+$ a b -
^\w+$ a　b -
\W aéb -
\W é- -
\W $'x\377y' -
\W $'\377' -
\W a\ b \ 
\W a b  
\W a　b 　
\<é aéb -
\<é é- é
\<é $'x\377y' -
\<é $'\377' -
\<é a\ b -
\<é a b -
\<é a　b -
a\> aéb -
a\> é- -
a\> $'x\377y' -
a\> $'\377' -
a\> a\ b a
a\> a b a
a\> a　b a
\bé aéb -
\bé é- é
\bé $'x\377y' -
\bé $'\377' -
\bé a\ b -
\bé a b -
\bé a　b -
é\B aéb é
é\B é- -
é\B $'x\377y' -
é\B $'\377' -
é\B a\ b -
é\B a b -
é\B a　b -
\s aéb -
\s é- -
\s $'x\377y' -
\s $'\377' -
\s a\ b \ 
\s a b -
\s a　b 　
é =~ ^[[:lower:]]$: [é]
É =~ ^[[:lower:]]$: [É]
é =~ ^[[:upper:]]$: [é]
ǆ =~ ǅ: [ǆ]
ǅ =~ ǆ: [ǅ]
ı =~ i: [ı]
ı =~ I: [ı]
I =~ ı: [I]
i =~ ı: [i]
ſ =~ s: [ſ]
ß =~ ẞ: no
xÉy =~ x.y: [xÉy]
aÉÉb =~ a(é+)b: [aÉÉb]
€ =~ [^[:alpha:]]: [€]
Straße =~ STRASSE: no
€ =~ [^[:alpha:]]: [€]
a中b =~ a[[:alpha:]]b: [a中b]
a中b =~ a[^[:alpha:]]b: no
x٣ =~ [[:digit:]]: no

== status 0
