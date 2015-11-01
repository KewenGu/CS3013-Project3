
Kewen Gu
Project 3
CS 3013
Oct 2, 2015

Introduction:
-------------

This is a multi-thread programming assignment. The analogy for this project is like this: there're a number of rats trying to traversing a sequence of rooms, while the rats are the threads created by the main process and the operation of traversing a sequence of rooms is the function which all threads should execute. Then record the time data for every rats that entering and leaving a room. Also, there're options can be set for the program to use different algorithms. The algorithms specify the order of rooms for the rat to traverse. There're three options for the traversing algorithms, i, d and n, which denotes in-order algorithm, distributed algorithm and non-blocking algorithm.

Instruction:
-------------

To compile the program maze.c:
	make
To run the program maze:
	./maze <number of rats> <travesing algorithm>


Discussion:
-------------

For the non-blocking algorithm (specified by -n), since the thread cannot be blocked, no semaphore is used in this algorithm. Instead, I used a while loop to do a conditional wait (evaluate a global variable to be false). If a room is already full and one rat wants to enter the room, this rat (thread) will execute an infinite loop until one rat left the room and change the value of the global variable, thus lead the while statement to be evaluated to be true. Then this rat is able to enter the room. The traversing order used for non-blocking algorithm is the same as the one used for the distributed algorithm. Finaly, the non-blocking algorithm yields a traversing speed to be as good as the in-order and distributed algorithms.
