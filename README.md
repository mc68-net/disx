disx multi-CPU diassembler
==========================

[disx], by [Bruce Tomlin], is an interactive, curses-based ([TUI]) tracing
disassembler for Unix systems, including Linux and MacOS. (It may work on
Windows via Cygwin or other means.)

It supports almsot two dozen different (mostly 8-bit) CPUs and variants,
including Z80, 6800, 6502, 6809, and so on. It writes a small binary
control file that can be committed to version control, along with an ASCII
symbol definition file and optional "equate" (for symbols with values
outside of the disassembly range) file. It can generate both source code
and listing files.

For further information, including a list of all CPUs and variants this
supports, see [the web site][disx] and [the documentation][disx4.txt].

### Branches

* `main`: Just this README.

* [`trunk`]: A `git svn` import of the [Subversion repo] as of early 2025.
  This branch is the trunk; the Git tags `disx4-*` are the tags from that
  repo, and there are no branches in it.

* [`doc-markdown`]: Based on `trunk`, this adds a `README.md` that more or
  less duplicates the [project home page][disx] and converts `disx4.txt` to
  Markdown as `USAGE.md` and `CHANGELOG.md`. (It also adds a `.gitginore`.)
  This brings the documentation to a state where it's displayed nicely by
  GitHub/GitLab/etc. It's also used as the base for feature fork branches.

### Updating `trunk`

For details of how git-svn works and conversion/import procedures, see
[sedoc:git/svn].

To update the `trunk` branch with the latest commits from the SVN repo,
check out this `main` branch and run the `update` script. This will set up
things and, due to the way that `git svn rebase` works, switch you to the
`trunk` branch, which does leave you in a convenient position to examine
and push the new commits.

`update` does not unfortunately yet update release tags, but that's a
little complex, requiring either `svn2git` (which has its own problems
related to branch naming) or doing some of the extra things that `svn2git`
does but `git svn` does not directly do.



<!-------------------------------------------------------------------->
[Bruce Tomlin]: http://xi6.com/
[TUI]: https://en.wikipedia.org/wiki/Text-based_user_interface
[disx4.txt]: http://svn.xi6.com/svn/disx4/trunk/disx4.txt
[disx]: http://xi6.com/projects/disx
[sedoc:git/svn]: https://github.com/0cjs/sedoc/blob/master/git/svn.md
[svn repo]: http://svn.xi6.com/svn/disx4/

[`doc-markdown`]: https://github.com/mc68-net/disx/tree/doc-markdown
[`trunk`]: https://github.com/mc68-net/disx/tree/trunk
