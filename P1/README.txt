Name: Leroy Souz
netID: lms548

-------------------------------------------------------------------------------------------------------------

*Compiling with makefiles*
memgrind(performance tests): $ make or $ make memgrind
memtests(correctess test): $ make memtests

-------------------------------------------------------------------------------------------------------------

TEST PLAN:

MetaData:
Chunk MetaData will contain size of chunk(including size of header), a flag that determines if chunk is 
free or not and a pointer to the next header.
A struct data struture will be used to keep track of MetaData

Malloc Implementation:
Malloc will take in a size variable and add the size of header struct to determine the size of chunk needed.
It will then iterate through all the available chunksto find a chunk that is big enough for the requested size.
If the chunk found is exactly equal to the requested size, a pointer to that chunk is returned by malloc. If
chunk is bigger, then the chunk is split into two, the first one is retured as a pointer to client and second
one is stored for latter use.

Free Implementation:
Free will take a pointer. It will do some basic checks to ensure that the pointer is valid.
These checks include:
    1) the pointer was one that was obtained from malloc
    2) Pointer is not free
    3) Pointer is at the start of the chunk

Once these checks are done free sets the free flag of the chunk header to true
 
The coalesce function will be run after this to merge free chunks

Coalescing:
We start with a pointer to the start of the memory array, this is where our initial header is stored. For the
chunk after the current chunk, two tests are performed, whether the chunk exists (not NULL) and whether its 
free, if these are true then we add the size of the next chunk to the current chunk and the adress of the 
chunk after the next chunk is set as the address of the chunk next pointer in our metadata, essentially 
deleting the chunk after the current chunk. Then we check if the chunk after the next chunk is free, if it is
then the pointer remains unchanged, if not then we change the pointer to point at the next header in the list.
This goes on until we reach the end of the memory array.

-------------------------------------------------------------------------------------------------------------

REPORTING ERRORS:

One the pointer was obtained from free, I did some pointer arithemtic to return a pointer to the 
header of the chunk.

1) Pointer not at the start of the chunk:
    In case the pointer was not at the start of the chunk, obtaining the metadata from the pointer would 
    return default values. Thus I checked if the header size returned was atleast more than the size of 
    the header struct (return value would be 0 if the address was not at the start of the chunk).
    The error was reported in the following format:
    Error while calling free:
    pointer not at the start of chunk (file:line)

2) Double Free:
    To prevent the client from freeing a pointer that was already freed before, I checked the metadata to
    check the status of the pointer being freed, if the pointer was already free, an error would be report
    The error was reported in the following format:
    Error while calling free:
    tried to free a pointer that is already free (file:line)

3) Pointer not obtained from malloc:
    To prevent the client from freeing a pointer not obtained from malloc, I tested to see if the pointer
    the client provided was within bounds of the memory array. If it was not then an error was reported.
    The error was reported in the following format:
    Error while calling free:
    pointer not obtained from malloc (file:line)

-------------------------------------------------------------------------------------------------------------

CORRECTNESS TESTING:

*Tests are present in memtest.c*

1) Testing if malloc and free worked properly:
    In this test i first tried to request a pointer that was the size of the whole memory array. To test if
    the malloc worked, it was checked if the returned pointer was NULL or not. The pointer was then 
    immediately freed. To test if free worked, another pointer, the size of the whole memory array was
    requested using malloc and checked if the function returned NULL.

    Results:
    Both tests passed

2) Testing if Free was doing required error reporting:

    a) Address not at the start of chunk:
        A pointer was requsted from malloc(), and then free was asked to free an address that was 1 byte 
        ahead of the address requested from malloc.

        Results:
        Proper error was reported
    
    b) Double free:
        A pointer was requested from malloc(), another duplicate pointer was created equal to the pointer
        requested from malloc. First the address obtained from malloc was freed and then the duplicate
        address was attempted to be freed

        Results:
        Proper error was reported

    c) Address not obtained from malloc():
        A double variable was created and then I tried to free the address of this variable using free()

        Results:
        Proper error was reported
    
3) Testing is blocks were properly coalesced:
    In this test I first requested two pointers from malloc(). Each were half the size of the memory. Both of
    these were immediately freed. Then another pointer was requested from malloc() which was the size of the
    whole memory array. If the coalesce() function worked right, malloc() would return an address, not NULL

    Results:
    An address was returned

-------------------------------------------------------------------------------------------------------------

ADDITIONAL STRESS TESTS:

1) Coalesce stress test:
    Request 120 1-byte pointers from malloc and store them in an array. Free alternate pointers to prevent any
    initial coalescing. Then free all remaining pointers. 

2) Random Size Stress test:
    We create an array of 75 pointers. We keep request malloc for pointers varying from size 1 to 55 until this
    array is full. When the malloc can't allocate any more space, we clear some pointers until malloc can
    allocate a new pointer. This goes on until the array of pointers is full.

-------------------------------------------------------------------------------------------------------------

Run times and Averages for Stress Tests:

PERFORMANCE TEST 1
Time taken(0): 0.000002 seconds
Time taken(1): 0.000002 seconds
Time taken(2): 0.000002 seconds
Time taken(3): 0.000002 seconds
Time taken(4): 0.000003 seconds
Time taken(5): 0.000002 seconds
Time taken(6): 0.000002 seconds
Time taken(7): 0.000002 seconds
Time taken(8): 0.000002 seconds
Time taken(9): 0.000002 seconds
Time taken(10): 0.000003 seconds
Time taken(11): 0.000002 seconds
Time taken(12): 0.000002 seconds
Time taken(13): 0.000002 seconds
Time taken(14): 0.000002 seconds
Time taken(15): 0.000002 seconds
Time taken(16): 0.000002 seconds
Time taken(17): 0.000002 seconds
Time taken(18): 0.000002 seconds
Time taken(19): 0.000002 seconds
Time taken(20): 0.000003 seconds
Time taken(21): 0.000002 seconds
Time taken(22): 0.000002 seconds
Time taken(23): 0.000002 seconds
Time taken(24): 0.000002 seconds
Time taken(25): 0.000001 seconds
Time taken(26): 0.000002 seconds
Time taken(27): 0.000001 seconds
Time taken(28): 0.000002 seconds
Time taken(29): 0.000002 seconds
Time taken(30): 0.000002 seconds
Time taken(31): 0.000002 seconds
Time taken(32): 0.000002 seconds
Time taken(33): 0.000002 seconds
Time taken(34): 0.000002 seconds
Time taken(35): 0.000002 seconds
Time taken(36): 0.000002 seconds
Time taken(37): 0.000002 seconds
Time taken(38): 0.000002 seconds
Time taken(39): 0.000002 seconds
Time taken(40): 0.000002 seconds
Time taken(41): 0.000003 seconds
Time taken(42): 0.000002 seconds
Time taken(43): 0.000001 seconds
Time taken(44): 0.000002 seconds
Time taken(45): 0.000002 seconds
Time taken(46): 0.000003 seconds
Time taken(47): 0.000002 seconds
Time taken(48): 0.000003 seconds
Time taken(49): 0.000002 seconds

AVERAGE TIME: 0.000002 seconds

PERFORMANCE TEST 2
Time taken(0): 0.000030 seconds
Time taken(1): 0.000028 seconds
Time taken(2): 0.000029 seconds
Time taken(3): 0.000028 seconds
Time taken(4): 0.000029 seconds
Time taken(5): 0.000028 seconds
Time taken(6): 0.000018 seconds
Time taken(7): 0.000019 seconds
Time taken(8): 0.000018 seconds
Time taken(9): 0.000019 seconds
Time taken(10): 0.000019 seconds
Time taken(11): 0.000019 seconds
Time taken(12): 0.000019 seconds
Time taken(13): 0.000019 seconds
Time taken(14): 0.000018 seconds
Time taken(15): 0.000019 seconds
Time taken(16): 0.000018 seconds
Time taken(17): 0.000019 seconds
Time taken(18): 0.000019 seconds
Time taken(19): 0.000019 seconds
Time taken(20): 0.000018 seconds
Time taken(21): 0.000019 seconds
Time taken(22): 0.000019 seconds
Time taken(23): 0.000019 seconds
Time taken(24): 0.000019 seconds
Time taken(25): 0.000019 seconds
Time taken(26): 0.000019 seconds
Time taken(27): 0.000019 seconds
Time taken(28): 0.000019 seconds
Time taken(29): 0.000019 seconds
Time taken(30): 0.000019 seconds
Time taken(31): 0.000019 seconds
Time taken(32): 0.000019 seconds
Time taken(33): 0.000019 seconds
Time taken(34): 0.000019 seconds
Time taken(35): 0.000019 seconds
Time taken(36): 0.000019 seconds
Time taken(37): 0.000019 seconds
Time taken(38): 0.000020 seconds
Time taken(39): 0.000019 seconds
Time taken(40): 0.000019 seconds
Time taken(41): 0.000019 seconds
Time taken(42): 0.000022 seconds
Time taken(43): 0.000023 seconds
Time taken(44): 0.000021 seconds
Time taken(45): 0.000019 seconds
Time taken(46): 0.000018 seconds
Time taken(47): 0.000019 seconds
Time taken(48): 0.000019 seconds
Time taken(49): 0.000028 seconds

AVERAGE TIME: 0.000020 seconds

PERFORMANCE TEST 3
Time taken(0): 0.000012 seconds
Time taken(1): 0.000011 seconds
Time taken(2): 0.000010 seconds
Time taken(3): 0.000009 seconds
Time taken(4): 0.000009 seconds
Time taken(5): 0.000009 seconds
Time taken(6): 0.000016 seconds
Time taken(7): 0.000012 seconds
Time taken(8): 0.000016 seconds
Time taken(9): 0.000012 seconds
Time taken(10): 0.000016 seconds
Time taken(11): 0.000015 seconds
Time taken(12): 0.000014 seconds
Time taken(13): 0.000038 seconds
Time taken(14): 0.000016 seconds
Time taken(15): 0.000013 seconds
Time taken(16): 0.000015 seconds
Time taken(17): 0.000013 seconds
Time taken(18): 0.000017 seconds
Time taken(19): 0.000015 seconds
Time taken(20): 0.000015 seconds
Time taken(21): 0.000014 seconds
Time taken(22): 0.000019 seconds
Time taken(23): 0.000015 seconds
Time taken(24): 0.000016 seconds
Time taken(25): 0.000014 seconds
Time taken(26): 0.000016 seconds
Time taken(27): 0.000016 seconds
Time taken(28): 0.000014 seconds
Time taken(29): 0.000013 seconds
Time taken(30): 0.000018 seconds
Time taken(31): 0.000014 seconds
Time taken(32): 0.000016 seconds
Time taken(33): 0.000016 seconds
Time taken(34): 0.000017 seconds
Time taken(35): 0.000014 seconds
Time taken(36): 0.000014 seconds
Time taken(37): 0.000021 seconds
Time taken(38): 0.000010 seconds
Time taken(39): 0.000014 seconds
Time taken(40): 0.000011 seconds
Time taken(41): 0.000011 seconds
Time taken(42): 0.000009 seconds
Time taken(43): 0.000010 seconds
Time taken(44): 0.000010 seconds
Time taken(45): 0.000009 seconds
Time taken(46): 0.000010 seconds
Time taken(47): 0.000009 seconds
Time taken(48): 0.000011 seconds
Time taken(49): 0.000013 seconds

AVERAGE TIME: 0.000019 seconds

PERFORMANCE TEST 4
Time taken(0): 0.000040 seconds
Time taken(1): 0.000039 seconds
Time taken(2): 0.000039 seconds
Time taken(3): 0.000038 seconds
Time taken(4): 0.000038 seconds
Time taken(5): 0.000038 seconds
Time taken(6): 0.000038 seconds
Time taken(7): 0.000038 seconds
Time taken(8): 0.000038 seconds
Time taken(9): 0.000038 seconds
Time taken(10): 0.000038 seconds
Time taken(11): 0.000039 seconds
Time taken(12): 0.000038 seconds
Time taken(13): 0.000038 seconds
Time taken(14): 0.000038 seconds
Time taken(15): 0.000039 seconds
Time taken(16): 0.000038 seconds
Time taken(17): 0.000039 seconds
Time taken(18): 0.000038 seconds
Time taken(19): 0.000038 seconds
Time taken(20): 0.000038 seconds
Time taken(21): 0.000033 seconds
Time taken(22): 0.000025 seconds
Time taken(23): 0.000025 seconds
Time taken(24): 0.000025 seconds
Time taken(25): 0.000026 seconds
Time taken(26): 0.000025 seconds
Time taken(27): 0.000025 seconds
Time taken(28): 0.000026 seconds
Time taken(29): 0.000026 seconds
Time taken(30): 0.000026 seconds
Time taken(31): 0.000026 seconds
Time taken(32): 0.000026 seconds
Time taken(33): 0.000025 seconds
Time taken(34): 0.000026 seconds
Time taken(35): 0.000026 seconds
Time taken(36): 0.000026 seconds
Time taken(37): 0.000025 seconds
Time taken(38): 0.000026 seconds
Time taken(39): 0.000025 seconds
Time taken(40): 0.000025 seconds
Time taken(41): 0.000025 seconds
Time taken(42): 0.000025 seconds
Time taken(43): 0.000025 seconds
Time taken(44): 0.000026 seconds
Time taken(45): 0.000025 seconds
Time taken(46): 0.000025 seconds
Time taken(47): 0.000026 seconds
Time taken(48): 0.000026 seconds
Time taken(49): 0.000026 seconds

AVERAGE TIME: 0.000032 seconds

PERFORMANCE TEST 5
Time taken(0): 0.000011 seconds
Time taken(1): 0.001837 seconds
Time taken(2): 0.000011 seconds
Time taken(3): 0.000009 seconds
Time taken(4): 0.000011 seconds
Time taken(5): 0.000011 seconds
Time taken(6): 0.000010 seconds
Time taken(7): 0.000011 seconds
Time taken(8): 0.000012 seconds
Time taken(9): 0.000009 seconds
Time taken(10): 0.000009 seconds
Time taken(11): 0.000128 seconds
Time taken(12): 0.000010 seconds
Time taken(13): 0.000011 seconds
Time taken(14): 0.001385 seconds
Time taken(15): 0.000011 seconds
Time taken(16): 0.000019 seconds
Time taken(17): 0.000009 seconds
Time taken(18): 0.000009 seconds
Time taken(19): 0.000009 seconds
Time taken(20): 0.000010 seconds
Time taken(21): 0.000009 seconds
Time taken(22): 0.000011 seconds
Time taken(23): 0.000009 seconds
Time taken(24): 0.000008 seconds
Time taken(25): 0.000014 seconds
Time taken(26): 0.000008 seconds
Time taken(27): 0.000011 seconds
Time taken(28): 0.000012 seconds
Time taken(29): 0.000011 seconds
Time taken(30): 0.000011 seconds
Time taken(31): 0.000012 seconds
Time taken(32): 0.000017 seconds
Time taken(33): 0.000010 seconds
Time taken(34): 0.000013 seconds
Time taken(35): 0.000008 seconds
Time taken(36): 0.230619 seconds
Time taken(37): 0.000013 seconds
Time taken(38): 0.000010 seconds
Time taken(39): 0.000009 seconds
Time taken(40): 0.000012 seconds
Time taken(41): 0.000018 seconds
Time taken(42): 0.000010 seconds
Time taken(43): 0.000010 seconds
Time taken(44): 0.000011 seconds
Time taken(45): 0.000012 seconds
Time taken(46): 0.000010 seconds
Time taken(47): 0.000010 seconds
Time taken(48): 0.019726 seconds
Time taken(49): 0.247765 seconds

AVERAGE TIME: 0.010043 seconds

---------------------------------------------END-------------------------------------------------------------