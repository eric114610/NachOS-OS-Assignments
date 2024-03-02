#include <fstream>
#include "thread.hpp"
#include "ts_queue.hpp"
#include "item.hpp"
#include "consumer_controller.hpp"

#include <unistd.h>
#include <chrono>
typedef std::chrono::high_resolution_clock CLOCK;

class Experiment : public Thread {
public:
	Experiment(std::string output_file, TSQueue<Item*>* worker_queue, ConsumerController* cc, int check_period);
	~Experiment();

	virtual void start() override;
private:

	std::ofstream ofs;
	TSQueue<Item*> *worker_queue;
    ConsumerController* cc;
    int check_period;
    bool cancel;

	static void* process(void* arg);
};


Experiment::Experiment(std::string output_file, TSQueue<Item*>* worker_queue, ConsumerController* cc, int check_period)
	: worker_queue(worker_queue), check_period(check_period), cc(cc){
	ofs = std::ofstream(output_file);
}

Experiment::~Experiment() {
	ofs.close();
}

void Experiment::start() {
    pthread_create(&t, 0, Experiment::process, (void*)this);
}

void* Experiment::process(void* arg) {
    Experiment* exp = (Experiment*)arg;
    auto start = CLOCK::now();
    while(!exp -> cancel){
        // int sz = exp -> worker_queue -> get_size();
        int sz = exp -> cc -> consumers.size();
        exp -> ofs << (double)(std::chrono::duration_cast<std::chrono::nanoseconds>(CLOCK::now() - start).count())/1000000000 << " " << sz << "\n";
        usleep(exp -> check_period);
    }
	return nullptr;
}




