# C-Reduce

## About 

C-Reduce is a tool that takes a large C or C++ program that has a
property of interest (such as triggering a compiler bug) and
automatically produces a much smaller C/C++ program that has the same
property.  It is intended for use by people who discover and report
bugs in compilers and other tools that process C/C++ code.

*NOTE:* C-Reduce happens to do a pretty good job reducing the size of
programs in languages other than C/C++, such as JavaScript and Rust.
If you need to reduce programs in some other language, please give it
a try.

Documentation and other information can be found at the [C-Reduce web
page](http://embed.cs.utah.edu/creduce/)

## Installation

See [INSTALL.md](INSTALL.md).

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

