// The executable's startup on POSIX, for the roots main_linux.cpp and
// main_darwin.cpp, after the platform layer.
// This is free and unencumbered software released into the public domain.

[[noreturn]] static void os_oom()
{
    if (sigos) flushw(sigos);
    Str msg = "bash: out of memory\n";
    writeall(2, msg.data, msg.len);
    _exit(2);
}

static OsVtable const *osvtable(Os *)
{
    return &pltvtable;
}

int main(int argc, char **argv)
{
    static Os os;
    os.werrfd = -1;
    OsInfo info = {};
    info.reserve = (iz)1 << (sizeof(void *)==8 ? 34 : 28);
    Arena a = newarena(&os, (iz)1<<26);
    Slice<InheritFd> in = inherited(&a);
    info.inherit  = in.data;
    info.ninherit = in.len;
    initasync(&os);
    info.nargs = argc;
    info.args  = alloc<Str>(&a, argc);
    for (i32 i = 0; i < argc; i++) {
        info.args[i] = fromcstr(argv[i]);
    }
    for (; environ[info.nenv]; info.nenv++) {}
    info.env = alloc<Str>(&a, info.nenv);
    for (iz i = 0; i < info.nenv; i++) {
        info.env[i] = fromcstr(environ[i]);
    }
    char *cwd = getcwd((char *)allocbytes(&a, 4096), 4096);
    info.cwd  = cwd ? fromcstr(cwd) : Str("/");
    char *exe = (char *)allocbytes(&a, 4096);
    ssize_t exelen = readlink("/proc/self/exe", exe, 4095);
    if (exelen > 0) {
        info.exe = {(u8 *)exe, (iz)exelen};  // Linux; elsewhere unknown
    }
    info.pid  = getpid();
    info.ppid = getppid();
    info.pgid = getpgrp();
    info.uid  = getuid();
    info.euid = geteuid();
    info.gid  = getgid();
    info.egid = getegid();
    char *host = (char *)allocbytes(&a, 256);
    if (!gethostname(host, 255)) {
        host[255] = 0;
        info.host = fromcstr(host);
    }
    info.machtype = machtype(&a);
    struct rlimit rl;
    if (!getrlimit(RLIMIT_STACK, &rl) && rl.rlim_cur!=RLIM_INFINITY) {
        info.stack = (iz)rl.rlim_cur;
    } else {
        info.stack = (iz)1<<23;
    }
    for (i32 i = 0; i < 3; i++) {
        info.std[i] = (Handle)i;
    }
    info.sigign = os.entryign;
    os.reserve  = info.reserve;
    os.stack    = info.stack;
    exitwith(sh_main(&os, &info));
}
