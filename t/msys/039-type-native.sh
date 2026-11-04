# vos
# Native programs on PATH: also listed without .exe; others keep theirs
type argv argv32
command -v argv
type -P argv.exe
type cc
command -v cmd
type -P CMD
command -v cmd.exe
hash -t argv
type nosuchprogram; echo "status $?"
command -v nosuchprogram; echo "status $?"
