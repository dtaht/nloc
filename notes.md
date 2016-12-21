Is the meta question - why is loccount a bad model, and what can be done
about it? Recursively, I set about improving loccount, and trying to
observe what I did while doing so.

I've never written a line of go in my life, but I've programmed in
(easily) 30 other programming languages, and go was such a small step
from C as to be able to easily start figuring it out. So I looked over
esr's code.

In under two hours, and not grokking go, I'd spotted an easy optimization.

My SSD can transfer 500MByte/sec and the linux kernel is 24MB in size,
so there was no I/O reason why the code should take much more than
50msec to run. In the age of SSDs, you don't have to worry much
about seek time either, and if cached, memory can transfer in 3Gbytes/sec.

We were surprisingly - not I/O bound at all.

The code, as of commit

two hours, two lines of code, halved the runtime on my dual core box,
and scaled as a number of cores on what he calls "the beast". Where my
processing time was 34 seconds for the linux kernel and 64 originally,
his was cut to 10.

The original sloccount code took 93 seconds on my machine, so this was a factor
of 3 speedup on cheap hardware - total win, time to quit, right?

Wellll...... Lets profile the go code, shall we?

FIXME

There are two things here that stand out - one, that it spends a LOT of time
in a regex. As jwz famously said: "When you decide a problem needs a regex to solve it, now you've got two problems."
I decided to ignore that one for now.

The second thing, though, is that it's spending an enormous amount of time
looking at one character at a time.

The classic getachar paradigm is really common "out there".

It's taught in all the computer science books. It's both an artifact
of stream processing being hard in the 70s, where having a mere block
of disk in core memory was a sizeable fraction thereof, and of the
per-character abstraction which permitted I/O from anywhere to anywhere.

But I had another tool in my toolbox - mmap. And the amount of indirection
being done here was really insane, there was no reason why merely accessing
memory (a single assembly language instruction) should take so long.

Did loccount

Is it worth optimizing? Well I lost the next 6 hours of my life and I couldn't figure out how to make mmap
in go work, so I tossed off a quick C version (copy paste from
here) and got it to basically work.

All the magic in go comes from being able to do CSP cleanly inside of one program.
CSP, however, dates back to the early days of unix, and we can reassemble a simple
one line shell script to basically emulate all of what loccount.go did:

A rough equivalent of what loccount does would become:

find * -type f -name \*.c -o -name \*.h | xargs -P 8 -x 2048 nloc

The magic bit - that scales to the number of cores, is the -P 8.

The -P option to xargs means the maximum number of processes to
run in parallel. It's not frequently used, because this makes the results in the
next stage in the pipeline indeterminate, and may lead to garbage unless you're careful,
But this  problem is embarrasingly parallel, so we're good there.

so I ran nloc-test on the net-next kernel on a different machine from which I started
and started to capture run-times on each.

## nloc v1

real	0m4.044s
user	0m11.600s
sys	0m1.300s

## vs loccount

real	0m24.652s
user	1m9.268s
sys	0m3.500s

So, even though the result is wrong, we might be able to get as much
as a 6fold speedup from doing things in C and not using so much
of the go library.

Sometimes you don't need to prove an alternate approach entirely correct
before tackling a potential optimization for it. Let's add mmap support,
since it's easy, to the same bit of c code and see if we can win.

## Oops
Program received signal SIGSEGV, Segmentation fault.
0x0000000000400a0c in countComments (fp=3, totalLines=0x7fffffff0d30,
    single=0x7fffffff0d38, multi=0x7fffffff0d40) at nloc2.c:113
113	    curAction = action[curState][c]; // Read the action before we overwrite the state
(gdb) up
#1  0x0000000000400bed in main (argc=2111, argv=0x7fffffff0e38) at nloc2.c:165
165	  countComments( fp, &totalLines, &single, &multi );

I forgot we have some utf-8 in there.

So I switched to just ignore that (for now, not knowing if I can find a good utf-8 lib to use)

And boom, using mmap was 30% faster.

real	0m2.833s
user	0m5.780s
sys	0m0.924s

## Ints are not always best

The size of the state table was quite large - 5*128*4 bytes for one and 4 for the other.
Often, switching the size of a lookuptable is bad, but in this case, I made it
also be unsigned chars.

Yea!

## FIXME, I forget the progress of things here.

real	0m1.842s
user	0m5.908s
sys	0m0.500s

real	0m3.528s
user	0m11.516s
sys	0m0.476s

We've proven we can get a wrong result 30x faster! We've shown that
mmap is roughly twice as efficient as using the file library
and character at a time processing.

## Short circuit the common case

After one more pass through the code, I realized that what
I'd blindly copy pasted, didn't short circuit the common case,
where there is no action.

So I put that in, and cut the runtime by another X.

real	0m1.607s
user	0m5.332s
sys	0m0.300s

Let's disable debugging and compile with -O3

real	0m0.805s
user	0m2.292s
sys	0m0.428s

## Can we stop now?

Well, there is no reason why the state table had to be in ints. Int
is a natural quantity but it's much better to keep the size of the
cacheline down:

And the answer to that was - YES - from a single threaded result of:

real	0m2.310s
user	0m2.052s
sys	0m0.252s

To a:

real	0m1.891s
user	0m1.584s
sys	0m0.296s

## And after applying -O3 again and doing it in parallel

We can scan the entire linux kernel in .668 of a second.

real	0m0.668s
user	0m1.892s
sys	0m0.424s

## Why is the go so slow?

I don't understand the ioReader abstraction. quantum cognative leap.

Well, after stopping work on saturday, I then spent two days mulling in my
mind over how to fix it. The test code I wrote I will probably throw away
entirely

or I can spend the time trying to deeply understand why go is so slow.

Another thing was made apparent to me, along the way, in that accessing
the precompiled regex is covered by a lock, so although we've efficiently
managed the regex compilation, it might be better to have a pool of
threads that each had their own copy.

500Mbytes/sec and the linux kernel is only a mere 24MB in size.

As someone that has had great difficulty getting paid, of late,
this is kind of important to me.

## TODO

1) shrink the size of the state and action tables ASCII starting at 10,
ending at / (47)
http://www.asciitable.com/

After:
real	0m0.850s
user	0m2.644s
sys	0m0.316s
dave@nemesis:~/git/linux/net-next$ time nloc-test
19689569

before:
real	0m0.661s
user	0m1.980s
sys	0m0.332s

branch predictor? compiler ors the top bit? what?

I hurredly backed it out.

Or

2) Go with an if statement rather than a state machine

Generally smaller instruction pipelines are better, a cascade
of if/then/else statements tends to overwhelm the OOO and branch
predictors.

## Easy language-isms

Go has sort and sortable and map types. C has none of these. A single
call to a sort is likely to be fraught with error.

## Tedium and structured programming

My divisor is inspiration vs tedium. Early in the morning I tackle the
tough problems, when otherwise

There was a lot of tedium in this port. Take argument parsing for
example. Every language does this differently. C, in particular,
evolved multiple different means of doing it, starting with the
venerable getopt, and then getopt_long. Then there was popt.h.
Libraries on top of C like glib added in default options, and
other additional processing... but, still doing it remains tedious.

Another thing I find horrifically tedious is the pace at adding
needed callbacks to a gui widget.

"structured programming"

## Todo items

Filewalking
Arg parsing
Structure assignment
Initialization
Counting by language
Regex compilation
UTF-8 handing

## The "Oh, god" stuff

sort, slices, strings, ranges, hashes, maps, and csp

## Arg parsing


getopt_long does not. it's ints or else.
