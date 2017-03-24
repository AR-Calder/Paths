#include <iostream>
#include <vector>
#include <thread>
#include <list>
#include <mutex>
#include <atomic>



using std::vector;
using std::list;
using std::cout;
using std::endl;
using std::thread;
using std::unique_lock;
using std::mutex;
using std::bind;
using std::condition_variable;
using std::atomic_bool;


class Tasker
{
public:
	typedef std::function<void()> task_;
	//Secure constructor
	static Tasker* create(int);

	//Constructor
	Tasker(int);

	//Destructor
	~Tasker();

	//Add new tasks to task list
	void add_task(task_);
	
	//have calling thread wait until tasks are completed
	void wait();

private:
	//get task, run task, remove task from list
	void Tasker::Run();

	vector<thread*> thread_vector;
	condition_variable run_cv;
	condition_variable end_cv;
	list<task_> task_list;
	atomic_bool exit;
	atomic_bool end_condition;
	mutex mx;

};

Tasker* Tasker::create(int fetchers)
{
	if (fetchers > 0) {
		return new Tasker(fetchers);
	}
	return nullptr;
}

Tasker::Tasker(int fetchers)//need some way of ensuring this is at least 1
{

	//initialise exit clause to false
	exit = false;
	//create number of threads specified
	for (auto i = 0; i < fetchers; ++i) {
		thread_vector.push_back(new thread(bind(&Tasker::Run, this)));
	}

}

Tasker::~Tasker()
{
	{
		unique_lock<mutex> lock(mx);
		//begin exit process
		exit = true;
		//notify run() that exit has been called
		run_cv.notify_one();
	}
	//join then destroy threads
	for (auto i : thread_vector) {
		i->join();
		delete i;
	}
	
}

//let threads know there are no more tasks to add
void Tasker::wait() {
	unique_lock<mutex> lock(mx);
	exit = true;
	end_cv.wait(lock, [&]() -> bool {return end_condition; });
}

//add tasks to task list
void Tasker::add_task(task_ task)
{
	//adding new items no need to exit!
	exit = false;

	unique_lock<mutex> lock(mx);
	//actually add tasks to task list
	task_list.push_back(task);
	//notify run that new task is available
	run_cv.notify_one();
}

void Tasker::Run()
{
	task_ this_task;

	while (true)
	{
		{
			unique_lock<mutex> wait_lock(mx);

			//wait until task list is not empty or exit has been called
			run_cv.wait(wait_lock, [&]() -> bool {return exit || !task_list.empty(); });

			if (exit && task_list.empty()) {
				end_condition = true;
				end_cv.notify_all();
				break;
			}
			//grab next task and remove it from list of tasks
			this_task = task_list.front();
			task_list.pop_front();
		}

		//do this task
		this_task();
	}
}
