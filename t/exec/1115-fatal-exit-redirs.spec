== stdout 2615
{ : ${u?}; } >out                                            script 1 out=[in-exit] file=[]
{ : ${u?}; } >out                                            -c     127 out=[in-exit] file=[]
f() { : ${u?}; } >out; f                                     script 1 out=[in-exit] file=[]
f() { : ${u?}; } >out; f                                     -c     127 out=[in-exit] file=[]
f() { : ${u?}; }; f >out                                     script 1 out=[in-exit] file=[]
f() { : ${u?}; }; f >out                                     -c     127 out=[in-exit] file=[]
{ eval ": \${u?}"; } >out                                    script 1 out=[] file=[in-exit]
{ eval ": \${u?}"; } >out                                    -c     127 out=[in-exit] file=[]
eval "{ : \${u?}; } >out"                                    script 1 out=[in-exit] file=[]
eval "{ : \${u?}; } >out"                                    -c     127 out=[in-exit] file=[]
echo ${ : ${u?}; }                                           script 1 out=[] file=[]
echo ${ : ${u?}; }                                           -c     127 out=[in-exit] file=[]
x=${ echo a; : ${u?}; }                                      script 1 out=[] file=[]
x=${ echo a; : ${u?}; }                                      -c     127 out=[in-exit] file=[]
{ echo ${ : ${u?}; }; } >out                                 script 1 out=[] file=[]
{ echo ${ : ${u?}; }; } >out                                 -c     127 out=[in-exit] file=[]
echo ${ { : ${u?}; } >out; }                                 script 1 out=[] file=[]
echo ${ { : ${u?}; } >out; }                                 -c     127 out=[in-exit] file=[]
echo ${ exec >out; : ${u?}; }                                script 1 out=[] file=[in-exit]
echo ${ exec >out; : ${u?}; }                                -c     127 out=[in-exit] file=[]
eval "echo \${ : \${u?}; }"                                  script 1 out=[in-exit] file=[]
eval "echo \${ : \${u?}; }"                                  -c     127 out=[in-exit] file=[]
echo ${| : ${u?}; }                                          script 1 out=[in-exit] file=[]
echo ${| : ${u?}; }                                          -c     127 out=[in-exit] file=[]
( trap "echo sub-exit" EXIT; { : ${u?}; } >out ); echo after script 0 out=[sub-exit
after
in-exit] file=[]
( trap "echo sub-exit" EXIT; { : ${u?}; } >out ); echo after -c     0 out=[after
in-exit] file=[sub-exit]
( trap "echo sub-exit" EXIT; echo ${ : ${u?}; } ); echo after script 0 out=[after
in-exit] file=[]
( trap "echo sub-exit" EXIT; echo ${ : ${u?}; } ); echo after -c     0 out=[after
in-exit] file=[]

== stderr 0

== merged 2615
{ : ${u?}; } >out                                            script 1 out=[in-exit] file=[]
{ : ${u?}; } >out                                            -c     127 out=[in-exit] file=[]
f() { : ${u?}; } >out; f                                     script 1 out=[in-exit] file=[]
f() { : ${u?}; } >out; f                                     -c     127 out=[in-exit] file=[]
f() { : ${u?}; }; f >out                                     script 1 out=[in-exit] file=[]
f() { : ${u?}; }; f >out                                     -c     127 out=[in-exit] file=[]
{ eval ": \${u?}"; } >out                                    script 1 out=[] file=[in-exit]
{ eval ": \${u?}"; } >out                                    -c     127 out=[in-exit] file=[]
eval "{ : \${u?}; } >out"                                    script 1 out=[in-exit] file=[]
eval "{ : \${u?}; } >out"                                    -c     127 out=[in-exit] file=[]
echo ${ : ${u?}; }                                           script 1 out=[] file=[]
echo ${ : ${u?}; }                                           -c     127 out=[in-exit] file=[]
x=${ echo a; : ${u?}; }                                      script 1 out=[] file=[]
x=${ echo a; : ${u?}; }                                      -c     127 out=[in-exit] file=[]
{ echo ${ : ${u?}; }; } >out                                 script 1 out=[] file=[]
{ echo ${ : ${u?}; }; } >out                                 -c     127 out=[in-exit] file=[]
echo ${ { : ${u?}; } >out; }                                 script 1 out=[] file=[]
echo ${ { : ${u?}; } >out; }                                 -c     127 out=[in-exit] file=[]
echo ${ exec >out; : ${u?}; }                                script 1 out=[] file=[in-exit]
echo ${ exec >out; : ${u?}; }                                -c     127 out=[in-exit] file=[]
eval "echo \${ : \${u?}; }"                                  script 1 out=[in-exit] file=[]
eval "echo \${ : \${u?}; }"                                  -c     127 out=[in-exit] file=[]
echo ${| : ${u?}; }                                          script 1 out=[in-exit] file=[]
echo ${| : ${u?}; }                                          -c     127 out=[in-exit] file=[]
( trap "echo sub-exit" EXIT; { : ${u?}; } >out ); echo after script 0 out=[sub-exit
after
in-exit] file=[]
( trap "echo sub-exit" EXIT; { : ${u?}; } >out ); echo after -c     0 out=[after
in-exit] file=[sub-exit]
( trap "echo sub-exit" EXIT; echo ${ : ${u?}; } ); echo after script 0 out=[after
in-exit] file=[]
( trap "echo sub-exit" EXIT; echo ${ : ${u?}; } ); echo after -c     0 out=[after
in-exit] file=[]

== status 0
