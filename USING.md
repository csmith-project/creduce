<h1>Using C-Reduce</h1>

<p>
This document is designed to be a gentle introduction to test-case
reduction using C-Reduce. More material like this can be found at
the <a href="http://delta.tigris.org/">web page for delta</a>, an
excellent tool from which C-Reduce has borrowed many ideas and
conventions.
</p>

<p>
Before getting started, you must have installed C-Reduce and it should 
be in your PATH.
</p>

<h2>Getting Started</h2>

<p>
Let's take a simple example: finding the smallest C program that
contains the string "goto". Grab any convenient C file that you have
sitting around that contains a goto or else
try <a href="crash/small.c">this
file</a>. Make sure your file is named <tt>small.c</tt> (there's
nothing special about this file name, it's just the one we'll use in
this document for files that want to be reduced).
</p>

<p>
Next we need an interestingness test: a program that returns 0 if a
partially-reduced test case is desirable, and returns non-zero
otherwise. Make a file called <tt>test1.sh</tt> with these contents:
</p>

<blockquote><b><pre>
#!/bin/bash
grep goto small.c &gt;/dev/null 2&gt;&amp;1
</pre></b></blockquote>

<p>
This script works because (according to grep's man page): "The exit
status is 0 if selected lines are found, and 1 if not found." 
</p>

<p>
Make <tt>test1.sh</tt> executable and ensure that it initially
succeeds:
</p>

<blockquote><b><pre>
$ chmod u+x test1.sh
$ ./test1.sh ; echo $?
0
</pre></b></blockquote>

<p>
Now run C-Reduce:
</p>

<blockquote><b><pre>$ creduce ./test1.sh small.c
</pre></b></blockquote>

<p>
C-Reduce should terminate in a few seconds with this at the
bottom of its output:
</p>

<blockquote><b><pre>reduced test case:

goto
</pre></b></blockquote>

<p>
The reduced test case is a single line containing only the string
"goto". C-Reduce has -- as greedy searches often do -- obeyed the
letter of the law but not the spirit. The lesson is that while
C-Reduce happens to be good at reducing C code, it contains no special
support for ensuring that its output is valid C. To fix this, let's
strengthen the interestingness test a bit:
</p>

<blockquote><b><pre>#!/bin/bash
gcc -c small.c &gt;/dev/null 2&gt;&amp;1 &amp;&amp;\
grep goto small.c &gt;/dev/null 2&gt;&amp;1
</pre></b></blockquote>

<p>
This interestingness test says that a test case is interesting if it
can be compiled by GCC (that is, the GCC process returns a "0" status
code to the operating system) and also it contains the string "goto".
</p>

<p>
Before running C-Reduce again, make sure to create a fresh copy of the
original C file. (In fact, it's usually best to perform each run of
C-Reduce in a fresh temporary directory.) This time the result is
better:
</p>

<blockquote><b><pre>reduced test case:

fn1 () {
lbl_1101:
  goto lbl_1101;
}
</pre></b></blockquote>

<p>
Before moving on to some real compiler bugs, let's try one more simple
example. Try reducing my example program above using this interestingness
test:
</p>

<blockquote><b><pre>gcc -c -Wextra small.c &gt;gcc_out.txt 2&gt;&amp;1 &amp;&amp;\
grep 'comparison is always false' gcc_out.txt &gt;/dev/null 2&gt;&amp;1 &amp;&amp;\
grep 'comparison is always true' gcc_out.txt &gt;/dev/null 2&gt;&amp;1
</pre></b></blockquote>

<p>
This test says that a program is interesting if it can be successfully
compiled, and also produces warnings including the two specified
strings.
</p>

<p>
Next, run C-Reduce. Your mileage may vary (warning behavior varies
across versions of GCC) but as output I get:
</p>

<blockquote><b><pre>unsigned char a;
fn1 () {
  ~0 &lt; a;
  295 &lt;= a;
}
</pre></b></blockquote>

<p>
This function does indeed elicit both warnings:
</p>

<blockquote><b><pre>$ gcc -Wextra -c small.c
small.c: In function 'fn1':
small.c:3:3: warning: comparison is always true due to limited range of data type [-Wtype-limits]
small.c:4:3: warning: comparison is always false due to limited range of data type [-Wtype-limits]
</pre></b></blockquote>

<p>
Can we create a smaller function by hand that satisfies this
interestingness test? Almost certainly so -- but not by much.
</p>


<h2>Reducing a Compiler-Crash Bug</h2>

<p>
Here we'll use a bug that causes the version of GCC (4.6.1) that is
distributed with Ubuntu 11.10 for x86-64 to crash. If you don't run
Ubuntu 11.10, you'll need to boot it up in a virtual machine or on a
spare box in order to go through this part of this tutorial. Images
can be found <a href="http://releases.ubuntu.com/11.10/">here</a>. The
version information for this compiler is:
</p>

<blockquote><b><pre>
$ gcc --version
gcc (Ubuntu/Linaro 4.6.1-9ubuntu3) 4.6.1
</pre></b></blockquote>

<p>
If the Ubuntu people push out a new version of GCC for 11.10, this
crash and the wrong code bug that follows may stop working. If that
happens please drop me (John Regehr) a line and I'll make some test
cases that break the new compiler.
</p>

<p>
Alternatively, you can build and install your own gcc-4.6.1 for
x86-64.
<a href="http://ftp.gnu.org/pub/gnu/gcc/gcc-4.6.1/gcc-core-4.6.1.tar.bz2">Source
code is here</a>.
</p>

<p>
The unreduced program (generated by Csmith) that crashes
this version of GCC is 
<a href="crash/small.c">the same one referenced above</a>.
Make sure that it does in fact cause GCC to crash:
</p>

<blockquote><b><pre>
$ gcc -w -O3 small.c
small.c: In function 'func_14.isra.10':
small.c:2727:1: internal compiler error: in simplify_subreg, at simplify-rtx.c:5226
Please submit a full bug report,
with preprocessed source if appropriate.
See <http://gcc.gnu.org/bugs.html> for instructions.
</file:></pre></b></blockquote>

<p>
Now reduce the original 167 KB program using this interestingness test:
</p>

<blockquote><b><pre>#!/bin/bash
gcc -c -O0 small.c &amp;&amp;\
! gcc -c -O3 small.c
</pre></b></blockquote>

<p>
Here we simply require that each intermediate program compiles
successfully at <tt>-O0</tt> but fails to compile at <tt>-O3</tt>.
</p>

<p>
The result should be something like:
</p>

<blockquote><b><pre>a;
b;
short c;
func_68 ();
func_14 ()
{
lbl_1651:
    a = c = a;
    b = (char) c ? (char) func_68 : 0;
    if (c)
        goto lbl_1651;
}
</pre></b></blockquote>

<p>
Although this code is small, our reduction was sloppy in two ways.
First, the interestingness test only specified that <tt>gcc -O3</tt>
crashed, but failed to look for the specific error
message <tt>internal compiler error: in simplify_subreg, at
simplify-rtx.c:5372</tt>. As it happens, this reduced test case still
triggers the crash of interest, but it is not uncommon to find that a
poorly-specified reduced test case triggers some entirely different
error than the desired one (this could be seen as a feature: C-Reduce
is a decently-powerful fuzzer). The second kind of sloppiness is
indicated by the lack of a type specifier for variables
<tt>a</tt> and <tt>b</tt> and for functions <tt>func_68()</tt> and
<tt>func_14()</tt>, as well as an undefined
behavior: <tt>func_14()</tt> should return an int, but fails to.
Compiler developers might fix a compiler crash triggered by this test
case, but they also might (legitimately) decline to acknowledge or fix
a bug triggered by a program with undefined behavior. It is strongly
recommended that test cases reported to compiler developers do not
contain undefined behaviors.
</p>

<p>
Both of these issues can be cleared up by refining the interestingness
test; see <a href="crash/test2.sh">this better one</a>, which uses
compiler warnings to reject partially reduced programs that contain
suspicious C code. Reducing this crash bug again with the new
interestingness test gives:
</p>

<blockquote><b><pre>
int a, b, c, d;
short e;
void
fn1 ()
{
lbl_1651:
    b = e = b;
    c = d || (char) e ? (char) e : 0;
    for (; a;)
        if (e)
            goto lbl_1651;
}
</pre></b></blockquote>

<p>
This code is much less objectionable and isn't much bigger. It is code
we would be happy to include in a bug report.
</p>

<h2>Reducing a Wrong Code Bug</h2>

<p>
A large test case that triggers a wrong-code bug in GCC 4.6.1 
for x86-64 can
be found
<a href="wrong1/small.c">here</a>.
</p>

<p>
First, let's ignore what we learned above about avoiding undefined
behaviors and go with a very simple interestingness test:
</p>

<blockquote><b><pre>
#!/bin/bash

ulimit -t 2

gcc -O1 small.c -o small1 >/dev/null 2>&1 &&\
./small1 >out1.txt 2>&1 &&\
gcc -O2 small.c -o small2 >/dev/null 2>&1 &&\
./small2 >out2.txt 2>&1 &&\
! diff out1.txt out2.txt
</pre></b></blockquote>

<p>
Reducers like to introduce infinite loops. It is critical to run
compiled versions of partially-reduced programs under a timeout. If
you don't do this, C-Reduce will just hang forever.
The <tt>ulimit</tt> command above limits all sub-processes to two
seconds of compute time. Any process that times out is killed by a
signal, causing it to return a non-zero status code, causing the
interestingness test to fail.
</p>

<p>
The purpose of the last five lines of this interestingness test is to
ensure that the both the broken compiler (<tt>gcc -O2</tt>) and a
reference compiler, which is assumed to not have the bug of interest,
can both compile the test case. Here our reference compiler is <tt>gcc
-O1</tt>, but in general we can use whatever C compiler we have
sitting around. This test also ensures that both of the resulting
executables return 0 when run, and that they emit output that
the <tt>diff</tt> tool does not consider to be the same.
</p>

<p>
Running C-Reduce using this interestingness test gives:
</p>

<blockquote><b><pre>
func_46 ()
{
}
main ()
{
    printf (func_46);
    return 0;
}
</pre></b></blockquote>

<p>
This code uses the address of a function as the format string argument
to printf. The level of wrongness here is gigantic. If we report this
as a test case triggering a compiler bug, we will be ignored at best.
</p>

<p>
A better interestingness test is <a href="wrong1/test1.sh">here</a>.
To avoid undefined behavior, it uses the same battery of warnings that
we used when reducing the crash bug above. Keep in mind that this
approach is inherently unreliable because compiler warnings are highly
unsound. You always need to carefully inspect the final output of any
test case reducer. Again, you never want to report a test case that
executes undefined behavior or relies on unspecified behavior. This is
particularly important for wrong-code bugs.
</p>

<p>
Now the result is:
</p>

<blockquote><b><pre>
int printf (const char *, ...);
struct S0
{
    int f0;
    int f1;
};
#pragma pack(1)
struct
{
    struct S0 f2;
    int f3;
    int f4;
    int f6:22;
} a =
{
    1
};
static char b = 1;
int c;
int
main ()
{
    a.f6 = 1;
    b = 0;
    for (; c < 10; c++)
        printf ("%d\n", b);
    return 0;
}
</pre></b></blockquote>

<p>
This program is not beautiful.
But is it valid C code that does not
execute undefined behavior or depend on unspecified behavior? It is.
Let's make sure it triggers the bug:
</p>

<blockquote><b><pre>
$ gcc -O1 small.c ; ./a.out 
0
0
0
0
0
0
0
0
0
0
$ gcc -O2 small.c ; ./a.out 
0
1
1
1
1
1
1
1
1
1
</pre></b></blockquote>

<p>
Since it meets the criteria, this test case appears to be reportable.
However, we won't always be so lucky. When compiler warnings fail to
screen out undefined behaviors, we can solve the problem using a 
heavy-weight program checker such as
<a href="http://frama-c.com/">Frama-C</a>: an open-source static
analyzer for C code that can be configured to act as a checking
interpreter. The full test script (which assumes that you have
installed the latest version of Frama-C) for this situation is
<a href="wrong1/test2.sh">here</a>. The 
<a href="http://code.google.com/p/c-semantics/">KCC</a> executable
semantics for C also makes an excellent undefined behavior checker.
</p>


<h2>Summary of Advice for Using C-Reduce</h2>

<ul>

<li>
Generally you want to run C-Reduce on a preprocessed file. There's no
inherent problem reducing a file that includes preprocessor
directives, but usually a test case that you report wants to be
self-contained.

<li>
If C-Reduce gives you crazy, undesirable results, you probably need to
refine your interestingness test. We suggest starting out using the
template tests provided with C-Reduce (in the <tt>scripts</tt>
subdirectory) and modifying them to fit your needs.

<li> Generally, each C-Reduce run should be performed in a fresh
temporary directory.

<li> C-Reduce will work poorly if the interestingness test is
non-deterministic. You'll need
to <a href="https://www.google.com/#hl=en&safe=off&output=search&sclient=psy-ab&q=%22turn+off+aslr%22">turn
off ASLR</a> if the compiler bug you are dealing with might involve a
memory safety violation.

<li> If the interestingness test runs compiled code, this must be
done under a time limit (and perhaps also under a memory limit).
Sometimes you will also need to run tools that process the current
test (compilers, static analyzers, etc.) under resource limits.

<li>
The interestingness test should run cheap parts of its test first. In
particular, the very first thing any test should do is try to exit as
rapidly as possible upon detecting a syntactically invalid program.

<li>
The interestingness test can filter out some kinds of undesirable
variants by inspecting compiler warnings.

<li>
Use a heavyweight undefined behavior checker to filter out undesirable
variants if compiler warnings aren't doing the job.

</ul>

<p>
Good luck, and please let us know about mistakes on this page and
bugs in C-Reduce.
</p>


<blockquote><b><pre></pre></b></blockquote>
