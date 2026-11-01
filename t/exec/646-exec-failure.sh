# exec failures: a found program that cannot run ends the shell without
# its EXIT trap, as a failed execve does; a name not found runs it
( trap 'echo trap1' EXIT; exec /nonexistent/x; echo no ); echo "st $?"
( trap 'echo trap2' EXIT; exec nonexistentcmd; echo no ); echo "st $?"
( trap 'echo trap3' EXIT; exec /tmp; echo no ); echo "st $?"
mkdir -p d; printf 'x' > d/noexec; chmod -x d/noexec
( trap 'echo trap4' EXIT; exec d/noexec 2>/dev/null; echo no ); echo "st $?"
( shopt -s execfail; trap 'echo trap5' EXIT; exec /nonexistent/y; echo "still here $?" ); echo "st $?"
( trap 'echo trap6' EXIT; exec true ); echo "st $?"
printf 'echo "no #! line: $1"\n' > d/script; chmod +x d/script
( trap 'echo trap7' EXIT; exec d/script arg; echo no ); echo "st $?"
shopt -s execfail
exec /nonexistent/w; echo "still here $?"
exec nonexistentcmd; echo "still here $?"
