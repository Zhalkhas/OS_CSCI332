# HW 2

Homework implements simple shell with primitive output redirection

## Requirements

Shell should:
* Present the prompt, “smsh% “
* Accept a command line of maximum length 254 chars
* Behave well if the user enters a non-existent or un-executable program, printing, “No such program: progname” where progname is whatever command the user entered.
* It should correctly handle the user entering any number of command-line arguments, up to a max of 100.
* It should be able to run a program in the background, accepting an ampersand ‘&’ as either the very last token, or as the last character of the very last token.  For example, both of the following should run the cat program in the background:
```bash
smsh% cat file.c&
smsh% cat file.c &
```

Your shell is required to recognize output redirection only if 
* the ‘>’ or ‘>>’ is an individual token (delimited on both sides by spaces)
* it appears as the next-to-last arg in an arglist, where the last arg is the filename
* or the second-from-last arg in the arglist if the very last arg is the & (see examples below)
For example, these are commands your shell must recognize and try to process as output redirection (whether they make sense or not):
```bash
smsh% cat file.c > ofile
smsh% cat file.c >> ofile
smsh% grep word infile > ofile
smsh% cat > infile > outfile
smsh% cat infile > >>
smsh% cat > file.c &
```
But the following do not have to be recognized as output redirection, or error-checked:
```bash
smsh% cat file.c>  ofile
smsh% cat file.c  >>ofile
smsh% cat > ofile infile
smsh% cat file.c > ofile >
```