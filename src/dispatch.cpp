// The shell's side of the platform table (api.cpp, OsVtable): each os_*
// entry calls the one of the same name in its Os's table. osvtable is
// the platform layer's. Where it returns a constant, as in the shell
// executables, every call folds into a direct one.
// This is free and unencumbered software released into the public domain.

static OsVtable const *osvtable(Os *);

static byte *os_reserve(Os *os, iz len)
{
    return osvtable(os)->reserve(os, len);
}

static b32 os_commit(Os *os, byte *p, iz len)
{
    return osvtable(os)->commit(os, p, len);
}

static void os_decommit(Os *os, byte *p, iz len)
{
    osvtable(os)->decommit(os, p, len);
}

static b32 os_write(Os *os, Handle h, u8 *buf, iz len)
{
    return osvtable(os)->write(os, h, buf, len);
}

static iz os_read(Os *os, Handle h, u8 *buf, iz len)
{
    return osvtable(os)->read(os, h, buf, len);
}

static Opened os_open(Os *os, Arena scratch, Str path, i32 flags, i32 mode)
{
    return osvtable(os)->open(os, scratch, path, flags, mode);
}

static Opened os_tempfile(Os *os, Arena scratch)
{
    return osvtable(os)->tempfile(os, scratch);
}

static NamedTemp os_namedtemp(Os *os, Arena *a, Str dir)
{
    return osvtable(os)->namedtemp(os, a, dir);
}

static void os_close(Os *os, Handle h)
{
    osvtable(os)->close(os, h);
}

static i32 os_unlink(Os *os, Arena scratch, Str path)
{
    return osvtable(os)->unlink(os, scratch, path);
}

static i32 os_rename(Os *os, Arena scratch, Str from, Str to)
{
    return osvtable(os)->rename(os, scratch, from, to);
}

static Handle os_dup(Os *os, Handle h)
{
    return osvtable(os)->dup(os, h);
}

static OsPipe os_pipe(Os *os)
{
    return osvtable(os)->pipe(os);
}

static NamedPipe os_namedpipe(Os *os, Arena *a, Str name, i32 how)
{
    return osvtable(os)->namedpipe(os, a, name, how);
}

static iz os_pipecap(Os *os)
{
    return osvtable(os)->pipecap(os);
}

static iz os_peek(Os *os, Handle h, u8 *buf, iz len)
{
    return osvtable(os)->peek(os, h, buf, len);
}

static i64 os_seek(Os *os, Handle h, i64 off, i32 whence)
{
    return osvtable(os)->seek(os, h, off, whence);
}

static Stat os_stat(Os *os, Arena scratch, Str path, b32 follow)
{
    return osvtable(os)->stat(os, scratch, path, follow);
}

static Str os_readlink(Os *os, Arena *a, Str path)
{
    return osvtable(os)->readlink(os, a, path);
}

static Stat os_fstat(Os *os, Handle h)
{
    return osvtable(os)->fstat(os, h);
}

static b32 os_isatty(Os *os, Handle h)
{
    return osvtable(os)->isatty(os, h);
}

static Tty os_tty(Os *os, Handle in, Handle out, i32 how)
{
    return osvtable(os)->tty(os, in, out, how);
}

static b32 os_con(Os *os, Handle out, ConOp *op)
{
    return osvtable(os)->con(os, out, op);
}

static Dirent *os_list(Os *os, Arena *a, Str dir, i32 *err)
{
    return osvtable(os)->list(os, a, dir, err);
}

static void os_cwd(Os *os, Arena scratch, Str dir)
{
    osvtable(os)->cwd(os, scratch, dir);
}

static Spawned os_spawn(Os *os, Arena scratch, Spawn *s)
{
    return osvtable(os)->spawn(os, scratch, s);
}

static ImageRef os_imagenew(Os *os, iz cap)
{
    return osvtable(os)->imagenew(os, cap);
}

static void os_imagefree(Os *os, ImageRef im)
{
    osvtable(os)->imagefree(os, im);
}

static i32 os_exec(Os *os, Arena scratch, Spawn *s)
{
    return osvtable(os)->exec(os, scratch, s);
}

static i32 os_forkexec(Os *os, Arena scratch, Spawn *s)
{
    return osvtable(os)->forkexec(os, scratch, s);
}

static i32 os_waitproc(Os *os, Handle proc)
{
    return osvtable(os)->waitproc(os, proc);
}

static void os_submit(Os *os, Io *io)
{
    osvtable(os)->submit(os, io);
}

static Io *os_wait(Os *os, i64 deadline)
{
    return osvtable(os)->wait(os, deadline);
}

static b32 os_cancel(Os *os, Io *io)
{
    return osvtable(os)->cancel(os, io);
}

static i32 os_kill(Os *os, i64 pid, i32 sig)
{
    return osvtable(os)->kill(os, pid, sig);
}

static i32 os_setpgid(Os *os, i64 pid, i64 pgid)
{
    return osvtable(os)->setpgid(os, pid, pgid);
}

static i64 os_tcpgrp(Os *os, Handle tty, i64 pgid)
{
    return osvtable(os)->tcpgrp(os, tty, pgid);
}

static Rlimit os_getrlimit(Os *os, i32 res)
{
    return osvtable(os)->getrlimit(os, res);
}

static i32 os_setrlimit(Os *os, i32 res, Rlimit lim)
{
    return osvtable(os)->setrlimit(os, res, lim);
}

static i32 os_siglevel(Os *os, i64 pid, i64 level, i32 sig)
{
    return osvtable(os)->siglevel(os, pid, level, sig);
}

static LevelSig os_levelsig(Os *os)
{
    return osvtable(os)->levelsig(os);
}

static u64 os_sigpending(Os *os)
{
    return osvtable(os)->sigpending(os);
}

static u64 os_siggroup(Os *os)
{
    return osvtable(os)->siggroup(os);
}

static void os_sigmode(Os *os, i32 sig, i32 mode)
{
    osvtable(os)->sigmode(os, sig, mode);
}

static i64 os_now(Os *os)
{
    return osvtable(os)->now(os);
}

static u64 os_entropy(Os *os)
{
    return osvtable(os)->entropy(os);
}

static ClipText os_clipget(Os *os, Arena *a)
{
    return osvtable(os)->clipget(os, a);
}

static i32 os_clipset(Os *os, Arena scratch, Str text)
{
    return osvtable(os)->clipset(os, scratch, text);
}

static Str os_localtz(Os *os, Arena *a)
{
    return osvtable(os)->localtz(os, a);
}

static void os_tasktag(Os *os, u64 tag, iz ntasks, u64 group)
{
    osvtable(os)->tasktag(os, tag, ntasks, group);
}

static void os_racy(Os *os)
{
    osvtable(os)->racy(os);
}
