# STF
Simple Task Fetcher - A simple framework for assigning tasks to threads farmer/worker style

Built for fun, but figured someone may find useful.

Example Usage:

//Create a tasker with 2 worker threads

Tasker foo(2);

//assign some tasks

foo.add_task([]() { cout << "this is an example task \n"; });

//wait for tasks to be completed

foo.wait()
