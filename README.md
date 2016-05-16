Compiled with g++ lab2.cpp -Wno-write-strings -lpthread
(May not need -Wno-write-strings)

Sources Used (other than TA/ professor): Computer Systems - A Programmer's Perspective by Bryant and O'Hallaron - Second Edition; Object-Oriented Data Structures Using Java Third Edition by Nell Dale, Daniel T. Joyce and Chip Weems; Introduction to Computer Science Using C++ Third Edition by Knowlton and Hunt; The C Programming Language Second Edition by Brian W. Kernighan and Dennis M. Ritchie; www.cplusplus.com; Stackoverflow.com: C++ Remove new line from multiline string; http://stackoverflow.com/questions/3501338/c-read-file-line-by-line; en.cppreference.com.   


/** Project Description **/


Short Description: From command prompt type "./MapReduce m n" where m is the number of mappers and n is the number of reducers. Make sure there are at least m files labeled foo1.txt to foom.txt. Each foo file needs a list of words (one per line).The program outputs a list of words along with the files and lines in which they appeared in the files. I have provided 3 foo files and three test cases. The test cases are labeled in the format MappersmReducersn.txt, where m was the number of foo files used and n was the number of reducers.


/** Design Details **/

My Map Reduce program uses an object oriented approach. The Classes are as follows:

Class Listing:
WordFileLine
BoundedBuffer
InvertedIndexEntry
InvertedIndex
Reducer
Mapper

These classes relate to each other as follows. The InvertedIndex is shared. An invertedIndex is made up of some number of invertedIndexEntry. The BoundedBuffers are global and there is one associated with each reducer. Making the BoundedBuffers globals was a design choice that exploits how well threads are able to share information. BoundedBuffers hold WordFileLine objects. They each have a bufferLock and the two condition variables: fullCondition and emptyCondition. The Mappers hash on a word to put it into a particular reducers associated BoundedBuffer. Reducers remove WordFileLine's from their associated BoundedBuffer and store them in a vector of InvertedIndexEntry. Once all the mappers complete and the Reducer has removed every WordFileLine from its BoundedBuffer, it gives its vector of InvertedIndexEntry over to the global InvertedIndex. The global InvertedIndex prints at the end. 

My Map Reduce program uses two threading methods. They are as follows:

Thread Methods:
map
reduce

Each map thread makes use of an instance of Mapper and read lines in a particular file. They make WordFileLine objects and hash them out to BoundedBuffers.

Each reduce thread makes use of an instance of Reducer and removes WordFileLine objects from its BoundedBuffer (when it has them). When it removes WordFileLine objects from its BoundedBuffer it creates an InvertedIndexEntry for storage in its InvertedIndexEntry vector. Once the reduce thread is finished, it gives its InvertedIndex Entry vector over to the global InvertedIndex. 

Synchronization Details:

Synchronization works by strategically placing locks to create critical sections in the right places. Conditions are also useful because they allow a thread to give up a particular lock and wait until a condition is signaled to acquire it again. Each BoundedBuffer had its own lock and conditions, to determine when the BoundedBuffer was full and empty and whether it was in use by another thread. The reduce threads wait on the empty condition and signal on the full condition. The map threads do the opposite.

The synchronization is modeled heavily on the Discussion 6 and 7 slides, with some small additions necessary to prevent deadlock. After the mappers had completed, it was necessary to signal the emptyCondition to all the BoundedBuffers. Otherwise it would be possible for some reduce threads to get stuck in a loop. In reducer threads it was also necessary to check that reducers with empty BoundedBuffers, yet non-empty InvertedIndexEntry vectors gave their entries over to the global InvertedIndex before exiting. This spot is commented on in the reduce method.


Test Cases:

I've included test cases along with the 3 foo files I used. Each test case is labeled with the number of Mappers and Reducers used in the test.


Improvements that need to be made:

(1)The hash function is very weak and could be improved. Currently the hash is based on the first character in a word. 

(2)Each reducer wastes time when it searches for a word in its own invertedIndex. This could be done with direct addressing. 