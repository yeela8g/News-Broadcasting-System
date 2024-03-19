# News Broadcasting System

## Introduction
The news broadcasting system is designed to efficiently manage and broadcast news items using a multi-threaded synchronization system in C. The system involves producers, a dispatcher, co-editors, and a screen manager, each performing specific tasks to manage and broadcast news items. The program utilizes pthreads for thread management, semaphores for synchronization, and dynamic memory allocation for buffer management. 

## Broadcasting Structure
![image](https://github.com/yeela8g/News-Broadcasting-System/assets/118124478/d08b9336-71aa-4e2f-b12c-a27c63299580)

The system comprises several entities, each with specific roles:

- **Producers**: These entities generate news items and enqueue them into bounded buffers for further processing. This system supports three producers, but its implementation lays the foundations for adapting the program to receive input with a variable number of producers.
- **Dispatcher**: Responsible for distributing news items to co-editors based on their types (e.g., sports, news, weather).
- **Co-Editors**: Each entity receives one type of news items from the dispatcher, performs editing tasks, and enqueues the edited items for display.
- **Screen Manager**: Manages the display of edited news items on the screen.

The system utilizes different types of buffers:
- **Bounded Buffers**: Used by producers and the co-editors to store a fixed number of news items. 
- **Unbounded Buffers**: Used by the dispatcher to store edited news items. These buffers dynamically resize to accommodate any number of items.


## Synchronization implementation

The synchronization in this program is crucial for ensuring that multiple threads can safely access shared resources, such as buffers, without causing data corruption or race conditions. Two primary synchronization mechanisms are employed: mutexes and semaphores.

### Mutexes

Mutexes (short for mutual exclusion) are used to ensure that only one thread can access a critical section of code at a time. In this program, mutexes are utilized to protect shared resources, particularly the buffer structures. By locking and unlocking mutexes, threads can safely manipulate the buffer contents without interference from other threads.

For example, when a producer thread wants to insert a message into a buffer, it first acquires the mutex associated with that buffer. This ensures that no other thread can access the buffer simultaneously. After inserting the message, the producer releases the mutex, allowing other threads to access the buffer.

### Semaphores

Semaphores are synchronization primitives used to control access to a shared resource by multiple threads. In this program, semaphores are employed to manage the flow of messages in the buffers, indicating when a buffer is full or empty.

For instance, each bounded buffer is associated with two semaphores: one to track the number of empty slots in the buffer (`empty` semaphore) and another to track the number of filled slots (`full` semaphore). When a thread wants to insert a message into a bounded buffer, it first waits on the `empty` semaphore. If the buffer has available space, the semaphore decrements, allowing the thread to insert the message. Conversely, when a thread removes a message from the buffer, it waits on the `full` semaphore. If the buffer contains messages, the semaphore decrements, allowing the thread to retrieve the message.

By coordinating access to buffers using mutexes and semaphores, this program ensures thread safety and prevents data inconsistencies, facilitating smooth communication between different components of the news broadcasting system.

## How to Run and Use
To compile the program, execute the following command:
```
make
```
Once compiled, run the program with the provided `config.txt` file as an argument:
```
./news_broadcasting.out config.txt
```

## Configuration File
The `config.txt` file specifies the details aboutu the co-editor's queue size and describes the necessary producer details.At first, Each block of three lines represents details for a producer, including the producer ID, the number of news items to generate, and the producer's bounded queue size. The final number in the configuration file represents the co-editor's bounded queue size. Finslly, the configuration file must end with a new line character. An example `config.txt` file is attached to thie repository and can be used and modified as needed. 

## Conclusion
The news broadcasting system demonstrates effective multi-threaded synchronization techniques in C programming. By utilizing mutexes and semaphores, the system ensures proper coordination among producers, dispatchers, co-editors, and the screen manager. Through the use of bounded and unbounded buffers, the system efficiently manages the flow of news items while preventing resource overflow.
