Name: Leroy Souz
NetID: lms548

--------------------------------------------------------------------------------

DESIGN:

Data Structure:
I decided to use a trie to store the dictionary due to a fixed lookup time for
each word, being the length of the word.

Implementation:

The nodes in the trie were structs that contained 3 data types:
    1) char* word
    2) Trie* children[27]

char* word:
This stored the pointer to the actual word from the dictionary (preserving the 
capitalization). Non leaf nodes had their pointer values set to NULL. This 
provided for easy checking for a word. After traversing the trie, we return
whatever value is stored by this pointer. If an actual word is returned, the 
word exists in the dict and if NULL is returned the word is invalid.

Trie* children[27]:
These are the sub-tries. Each sub-trie corresponds to a character where 0 is a,
1 is b and so on until 25 is z. the 26th branch is for handling (') and invalid
characters. All characters of the word are turned to lower case before the word
is inserted in the trie. The actual word with capitalization is stored in the 
word pointer so we don't lose capitalization. This way we save space by not
making unnecessary braches while still not losing the actual word.

Loading the dictionary file to the Trie:
After opening the dictionary file, we read the words from the dictionary to the
buffer. The data from this buffer is then copied to a string. We continue this
process until everything is read from the dictionary file. The string storing 
all the words are then tokenized. Each token is then inserted into the Trie.

Spell Check:
Just like the dictionary, I read the text file and store it in a string. This
is then tokenized to give seprate words. Each one of these words are then 
searched for in the Trie. After the word is returned, I generate the three
capitalization variations for the word. The original word is then checked to
atleast match one of these variations. If there is a match the word is spelled
correctly.

-------------------------------------------------------------------------------

TESTING

* all testing documents are stoered in the 'testfiles' folder *
* the linux standard dictionary 'words' was used for testing *

#File handling: (Files used: doc.txt, exitstatus0.txt, exitstatus1.txt)

To check if files are handle properly including opening and reading from them,
I test multiple files and check if the program terminates due to being unable
to open or read a file. We also print the data read from the files to ensure
everything is being read.
RESULT: All files were opened sucessfully and all the data was printed

#Directory Traversal: (Folder used: directory)

To test this we create a folder 'files' with subfolders than contain files. We
also create files that start with a '.' or do not end with txt. We print the 
files being traversed through and also print if the code is going to open them
for reading. If the code works right then each file from the folder should be 
printed and files starting with '.' and not ending with txt should not be 
opened for reading.
RESULT: All the files in the directory were successfully examined

#Spell Check Testing: (Files used: doc.txt)

Common Requirement testing:
1) Punctuation: 
{[("'hello@%^'" -> should strip down to hello and return as correct spelling
{/@hello} -> should strip down to /@hello and return as misspelled
he@llo -> should strip down to he@llo and return as misspelled

2) Hyphens:
almost-correct -> should return as correct
almost-korrect -> should return as misspelled
almest-kerrect -> should return as misspelled

3) Capitalization:
WiFi -> accepted
WIFI -> accepted
wifi -> rejected
hello -> accepted
heLlo -> rejected

3) General Testing:
Paragraph testing: The bueatiful sunset glowed orangey across the horizen, 
casting long shadows over the sand. The waves lapped gently at the shore, 
creating a relaxing atmospere. Seagulls sqwaked overhead as they soared through
the sky,searching for their evening meal. It was a tranquil scene, perfect for 
contemplation and reflaction. -> should return bueatiful, orangey, horizen, 
sqwaked, reflaction, seagull


4@ -> should not return anything since its not a word, only punctuation


ikorrect     inkorrect

inkorrect -> should report the second word 4 columns apart and third word 2
lines apart


Directory Traversal: (Folder used: directory)
Should find and print the word "inkorrectly" in every
file of the directory except for ones not ending in txt or starting with "."

RESULTS: All words return expected errors.

Exit Status testing: (Files used: exitstatus0.txt, exitstatus1.txt)

To test the exit status, we use two files:
exitstatus0.txt -> file with no mispelled words
exitstatus1.txt -> file with a mispelled word

After running spchk.c on both of these files we use the command: $ echo $? to
print the exit value from the code.
RESULTS: exitstatus0.txt returned 0(EXIT_SUCCESS) and exitstatus1.txt returned
1(EXIT_FALUIRE).