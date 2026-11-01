== stdout 1670
(|a)               a        <a><a>
(a|)               a        <a><a>
(a*)*              b        <><>
(a?)*              aa       <aa><a>
(a|ab)(c|bcd)(d*)  abcd     <abcd><a><bcd><>
(wee|week)(knights|night) weeknights <weeknights><wee><knights>
(a*)+              b        <><>
((a)|b)*           ab       <ab><b><a>
(x?){2}            x        <x><>
(.)(.)?(.)?        ab       <ab><a><b><>
x*                 zz       <>
\w+                --ab_1-- <ab_1>
\W+                ab--cd   <-->
\s+\S              a\ \ b   <  b>
\bab               xab\ ab  <ab>
ab\b               abx\ ab  <ab>
\<a.               ba\ ab   <ab>
.a\>               ab\ ba   <ba>
\`a                aa       <a>
a\'                aa       <a>
a*\B               ca.      <>
b*\B               _b       <>
\B                 .ab      <>
[]a]+              x\]a\]   <]a]>
[^]a]              \]\]b    <b>
[a-]+              x-a-     <-a->
[[.-.]]            a-b      <->
[[=a=]]            bab      <a>
[[:digit:][:punct:]]+ x1\,2y   <1,2>
a{2}               aaa      <aa>
a{,2}              aaa      <aa>
a{0}b              ab       <b>
a{1,3}?            aaaa     <aaa>
(a){0}\1           a        no 1
(a*)\1             aaaa     <aaaa><aa>
(a|b)*\1           abb      <abb><b>
(a?){2}\1          aa       no 1
)                  a\)b     <)>
}                  a\}b     <}>
a$.                $'a\nb'  <a
>
(a)$.              $'a\nb'  no 1
(.)^b              $'a\nb'  <
b><
>
^b                 $'a\nb'  no 1
(^a)+              aa       no 1
(\ba)+             aa       <aa><a>
(^a|b)+            aab      no 1
(a(b)?)+           aba      <aba><a><b>
(a|b){2,3}         ababa    <aba><a>

== stderr 0

== merged 1670
(|a)               a        <a><a>
(a|)               a        <a><a>
(a*)*              b        <><>
(a?)*              aa       <aa><a>
(a|ab)(c|bcd)(d*)  abcd     <abcd><a><bcd><>
(wee|week)(knights|night) weeknights <weeknights><wee><knights>
(a*)+              b        <><>
((a)|b)*           ab       <ab><b><a>
(x?){2}            x        <x><>
(.)(.)?(.)?        ab       <ab><a><b><>
x*                 zz       <>
\w+                --ab_1-- <ab_1>
\W+                ab--cd   <-->
\s+\S              a\ \ b   <  b>
\bab               xab\ ab  <ab>
ab\b               abx\ ab  <ab>
\<a.               ba\ ab   <ab>
.a\>               ab\ ba   <ba>
\`a                aa       <a>
a\'                aa       <a>
a*\B               ca.      <>
b*\B               _b       <>
\B                 .ab      <>
[]a]+              x\]a\]   <]a]>
[^]a]              \]\]b    <b>
[a-]+              x-a-     <-a->
[[.-.]]            a-b      <->
[[=a=]]            bab      <a>
[[:digit:][:punct:]]+ x1\,2y   <1,2>
a{2}               aaa      <aa>
a{,2}              aaa      <aa>
a{0}b              ab       <b>
a{1,3}?            aaaa     <aaa>
(a){0}\1           a        no 1
(a*)\1             aaaa     <aaaa><aa>
(a|b)*\1           abb      <abb><b>
(a?){2}\1          aa       no 1
)                  a\)b     <)>
}                  a\}b     <}>
a$.                $'a\nb'  <a
>
(a)$.              $'a\nb'  no 1
(.)^b              $'a\nb'  <
b><
>
^b                 $'a\nb'  no 1
(^a)+              aa       no 1
(\ba)+             aa       <aa><a>
(^a|b)+            aab      no 1
(a(b)?)+           aba      <aba><a><b>
(a|b){2,3}         ababa    <aba><a>

== status 0
