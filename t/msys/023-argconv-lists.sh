# vos
# POSIX path lists become ';' lists; the misfires are part of the rules
argv /a:/b /c/x:/tmp '/c/x;/d/y' /foo,/bar /x/.. '/a b' /a:b /a:/b: \
     //c //server/share //server '//c/x' /C: /c:/x '\x\y' '\\server\share'
