WHAT YOU CAN DO
make a gcc package.
make a wget package.

# Making a Package for pls

So you want to add a package to pls. Good news: right now it's about as
simple as it gets. There's no build system to learn, no config files, no
weird metadata format. If you can write a C file that compiles, you can
make a package.

## How pls actually finds your package

When someone types:

```
pls -i somepackage.c
```

pls just glues that filename onto the end of the repo's base URL and
downloads whatever's sitting there with curl. That's the whole trick.
Right now the base URL is:

```
https://raw.githubusercontent.com/official-butterOS/packages/refs/heads/main
```

So typing `pls -i somepackage.c` grabs:

```
https://raw.githubusercontent.com/official-butterOS/packages/refs/heads/main/somepackage.c
```

Which means, practically speaking, "making a package" just means "putting a
file in that repo." There's no submission process beyond that.

## What counts as a package right now

Honestly, a package is just a C file. No manifest, no folder, no version
number attached to it — pls downloads your file, saves it under whatever
name the person typed, and compiles it:

```
gcc somepackage.c -o somepackage.c_compiled
```

Whatever your `main()` does when that runs is what the package does. It's
that direct.

## How to actually make one

Write a normal C program. Nothing fancy required — just make sure it
compiles cleanly on its own with a plain `gcc yourfile.c`, since pls
doesn't pass any special flags when it builds things. If your code needs
something beyond the standard library, mention that clearly, because pls
isn't going to go install it for anyone.

Name the file something short and sensible, all lowercase, no spaces.
Whatever you name it is literally what people will type after `pls -i`, so
don't make it annoying to type.

Once it's written, drop it into the packages repo on the branch pls is
pointed at (currently main).

Before you consider it done, actually test it the way a real user would:

```
pls -i yourfile.c
```

Watch it download, watch it compile, and actually run the result to make
sure it does what you think it does.

## About updates

When someone runs `pls -u`, it goes through every package they've
installed and just repeats the install step for each one — download it
again, compile it again. There's no version checking happening under the
hood. If you push a change to your package's file in the repo, the next
time anyone runs `pls -u`, they'll get your new version automatically.

## A few things worth knowing

If you rename your package's file in the repo, anyone who installed it
under the old name is stuck — pls only knows the name it was told to
track, so renaming quietly breaks updates for existing users. Don't rename
things once they're out there unless you're okay with that.

There's no dependency system yet, so if your package needs something else
to work, just say so at the top of the file in a comment. pls isn't going
to figure that out or install it for someone.

Also worth remembering: pls will happily compile and run whatever you give
it, no questions asked. Anyone can read your source before they install
it, and honestly that's the only real safety check that exists right now,
so don't be sketchy about what your code does.

This is a pretty bare-bones setup on purpose. If pls ever grows a real
package format with versions or dependencies, this doc will get updated to
match.
