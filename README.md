# C-Reduce

## About 

C-Reduce is a tool that takes a large C or C++ program that has a
property of interest (such as triggering a compiler bug) and
automatically produces a much smaller C/C++ program that has the same
property.  It is intended for use by people who discover and report
bugs in compilers and other tools that process C/C++ code.

A pair of blog posts about the design and evolution of C-Reduce ([part
1](https://blog.regehr.org/archives/1678) and [part
2](https://blog.regehr.org/archives/1679)) is a good place to start
understanding how it works.

However, there is also an academic paper describing C-Reduce:
[Test-Case Reduction for C Compiler
Bugs](http://www.cs.utah.edu/~regehr/papers/pldi12-preprint.pdf) by
John Regehr, Yang Chen, Pascal Cuoq, Eric Eide, Chucky Ellison, and
Xuejun Yang. It was published in Proceedings of the 33rd ACM SIGPLAN
Conference on Programming Language Design and Implementation (PLDI
2012).

*NOTE:* C-Reduce happens to do a pretty good job reducing the size of
programs in languages other than C/C++, such as JavaScript, Rust, or
SMT-LIB.  If you need to reduce programs in some other language,
please give it a try!

## Using

Run C-Reduce like this:

```
creduce ./test1.sh file.c
```

Where `file.c` is a C compilation unit that you wish to reduce and
`test1.sh` is a shell script that returns 0 if a partially reduced
file (a "variant") is a suitable basis for further reduction steps (it
triggers the bug, or whatever) and returns non-zero otherwise.

The C-Reduce help output is another place to turn to for information:

```
creduce --help
```

For more details, see [USING.md](USING.md)

## Installation

See [INSTALL.md](INSTALL.md)

## Notes

1. When set to use more than one core, C-Reduce can cause space in
`/tmp` to be leaked. This happens because sometimes C-Reduce will kill
a compiler invocation when a result that is computed in parallel makes
it clear that that compiler invocation is no longer useful. If the
compiler leaves files in `/tmp` when it is killed, C-Reduce has no way
to discover and remove the files. You will need to do this manually
from time to time if temporary file space is limited. The leakage is
typically pretty slow. If you need to avoid this problem altogether,
you can run C-Reduce on a single core (using `--n 1`) in which case
C-Reduce will never kill a running compiler instance. Alternatively, a
command line option such as `-pipe` (supported by GCC) may suppress
the creation of temporary files altogether. Another possibility is to
set the `TMPDIR` environment variable to something like
`/tmp/creduce-stuff` before invoking C-Reduce -- assuming that the
tools you are invoking respect this variable.

2. Each invocation of the interestingness test is performed in a fresh
temporary directory containing a copy of the file that is being
reduced. If your interestingness test requires access to other files,
you should either copy them into the current working directory or else
refer to them using an absolute path.

## Contact

There are two mailing lists for C-Reduce:
[creduce-dev](http://www.flux.utah.edu/mailman/listinfo/creduce-dev)
for developer discussion, and
[creduce-bugs](http://www.flux.utah.edu/mailman/listinfo/creduce-bugs)
for reporting bugs.
