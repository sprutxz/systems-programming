Leroy Souz (lms548)
Mannan Shukla (ms3389)

--------------------------------------------------------------------------------
FIX: grep "Hello World"
DESIGN

Initializing batch or interative mode:
When the program is run, we check if there is an argument file provided and open
it. We pass this file descriptor as an arg to the input loop. If no file is
provided, we pass the file descriptor for stdin as the arg.

Input loop:
Using isatty() we check if the input file descriptor is a terminal input (inter-
active mode) or a text/shell file (batch mode).
For interactive mode, we read from the stdin until we receive a '\n', then parse
whchever string we have recieved.
For batch mode, we read each string line by line and parse the string.

Parsing Algorithm:
The parse string function takes two inputs, the string and last return value. It
check if the string contains a then or else conditional, if it does then it 
checks the last return value to determine if the current string should be parsed
or not.
If the command is to be parsed, the algorithm first goes over the string to
extract all the file redirection data and weather the string contains pipes.
After the data is extracted, the inspect the command string to count the total
args for the respective commands.
We then pass this string into a tokenization algorithm that returns a char** ptr
to the tokens. In case of the pipe, we pass the second half of the string to 
retrieve the tokens of the piped command. After we have recieved the tokens we 
check if the first token is a command we hvae built in. If not we do a search 
for its executable. We fork a child process and run this executable with its 
arguments. In case of a pipe we fork another child process and reepeat the 
process for the tokens of the second command. We return the exit status for use
to the next call of this parsing algorithm.

--------------------------------------------------------------------------------

TESTING

Mode testing:
We test if the correct modes run by first not passing any arg into the mysh exec
then we pass a shell script as an argument. We also test a case where the input
is from a pipe.
Results :-
$ ./mysh -> runs interactive mode
$ ./mysh batch.sh -> runs batch mode
$ cat batch.sh | ./mysh -> runs batch mode

Interactive Mode (Requirements testing):

Wildcards
We use the cat command to test if all wildcards are included. If everything is
correct, cat will print the contents of all the files fetched from the wildcard.
Results :-
mysh> cat testfiles/*.txt -> prints contents of 'doc.txt' and 'doc2.txt' and not
the contents of '.doc.txt'
mysh> cat testfiles/glob*.txt -> prints the contents of 'glob*.txt' and glob.txt

Redirection
We again use cat to test redirection. Using cat we take input or output and both
Results :-
mysh> cat > out.txt -> prints "this is what is being printed out" (input from
shell) to out.txt
mysh> cat < in.txt -> prints "this is what is being inputted" (contents of 
in.txt) to the stdout.
mysh> cat > out.txt < in.txt -> prints "this is what is being inputted" to
out.txt

Builtin Functions
To test the built in function we run them and ensure that work right.
Results :-
mysh> pwd -> prints the current directory
mysh> cd .. -> prints nothing
mysh> pwd -> prints the home directory
mysh> cd arg1 arg2 -> prints error saying too many args
mysh> cd notarealdir -> prints chdir() error
mysh> which gcc -> prints "/usr/bin/gcc"
mysh> which gcc cat -> prints nothing (too many args)
mysh> which cd -> prints nothing
mysh> which ./hw -> prints nothing (executable)
*exit testing later*

Pipes
To test pipes we print a output and grep a string from that 
Results :- 
mysh> ./hw | grep "Hello" -> prints "Hello, World!"

Batch Mode:
We use the batch.sh file to test for multiple conditions and ensure everything
prints right

Exit Testing:
We provide certain args and ensure they print are printed
mysh> exit arg1 arg2 arg3 -> prints "arg1 arg2 arg3"
mysh> exit arg1 arg2 arg3>out.txt -> prints the args to out.txt

Additional Testing
Redirection taking precedence over pipes
mysh> ls | cat < testfiles/in.txt
