Sample README.txt

Eventually your report about how you implemented thread synchronization
in the server should go here


<Contributions>
Yoohyuk Chang: worked on receiver.cpp and sender.cpp
Yongjae Lee: worked on connection.app and sender.cpp

<Synchronization Report>

In our server implementation, critical sections were identified as room 
management and message queue operations because of their shared access by 
multiple threads. Room management involves creating and managing chat rooms,
while message queues in user objects are accessed by sender and receiver threads.
To ensure thread-safe operations, we used mutex locks for room management, providing 
exclusive access to the room and preventing modifying something inside when 
concurrency is happening. For message queue operations, the Guard class was chosen 
for its automatic lock management, simplifying lock acquisition and release, especially 
in error scenarios or when functions end.

Our approach to synchronization effectively prevents race conditions and deadlocks.
Mutex locks in room management ensure that only one thread can modify the room at 
a time, thus maintaining data integrity and avoiding the race conditions.
In message queue operations, the guard class automatically releases locks, where it 
reduces the risk of deadlocks. Additionally, by applying locks only in necessary areas,
we ensure more efficient and reliable server operation.
