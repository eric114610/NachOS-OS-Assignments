#include <pthread.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include "consumer.hpp"
#include "ts_queue.hpp"
#include "item.hpp"
#include "transformer.hpp"

#include <ctime>
#include <chrono>
typedef std::chrono::high_resolution_clock CLOCK;

#ifndef CONSUMER_CONTROLLER
#define CONSUMER_CONTROLLER

class ConsumerController : public Thread {
public:
	// constructor
	ConsumerController(
		TSQueue<Item*>* worker_queue,
		TSQueue<Item*>* writer_queue,
		Transformer* transformer,
		int check_period,
		int low_threshold,
		int high_threshold
	);

	// destructor
	~ConsumerController();

	virtual void start();
    std::vector<Consumer*> consumers;
private:
	// std::vector<Consumer*> consumers;

	TSQueue<Item*>* worker_queue;
	TSQueue<Item*>* writer_queue;

	Transformer* transformer;

	// Check to scale down or scale up every check period in microseconds.
	int check_period;
	// When the number of items in the worker queue is lower than low_threshold,
	// the number of consumers scaled down by 1.
	int low_threshold;
	// When the number of items in the worker queue is higher than high_threshold,
	// the number of consumers scaled up by 1.
	int high_threshold;

	static void* process(void* arg);
};

// Implementation start

ConsumerController::ConsumerController(
	TSQueue<Item*>* worker_queue,
	TSQueue<Item*>* writer_queue,
	Transformer* transformer,
	int check_period,
	int low_threshold,
	int high_threshold
) : worker_queue(worker_queue),
	writer_queue(writer_queue),
	transformer(transformer),
	check_period(check_period),
	low_threshold(low_threshold),
	high_threshold(high_threshold) {
}

ConsumerController::~ConsumerController() {}

void ConsumerController::start() {
	// TODO: starts a ConsumerController thread
    pthread_create(&t, 0, ConsumerController::process, (void*)this);
}

void* ConsumerController::process(void* arg) {
	// TODO: implements the ConsumerController's work
    ConsumerController *c_controller = (ConsumerController*)arg;
    // printf("ConsumerController process\n"); // debug
    // printf("low = %d, high = %d\n", c_controller -> low_threshold, c_controller -> high_threshold);
    // experiments
    // clock_t start = clock();
    auto start = CLOCK::now();
    while(1){
        int sz = c_controller -> worker_queue -> get_size();
        // printf("checking... sz = %d, #consumers = %d\n", sz, c_controller -> consumers.size());
        if(sz < c_controller -> low_threshold && c_controller -> consumers.size() > 1){
            c_controller -> consumers[c_controller -> consumers.size()-1] -> cancel(); // delete consumer controller
            c_controller -> consumers.pop_back();

            printf("scaling down consumers from %d to %d\n", c_controller -> consumers.size()+1, c_controller -> consumers.size());

            //printf("scaling down consumers from %d to %d, %lf\n", c_controller -> consumers.size()+1, c_controller -> consumers.size(), (double)(clock()-start)/CLOCKS_PER_SEC);
            // printf("size = %d\n", sz);
            // printf("scaling down consumers from %d to %d, %lf\n", c_controller -> consumers.size()+1, c_controller -> consumers.size(), (double)(std::chrono::duration_cast<std::chrono::nanoseconds>(CLOCK::now() - start).count())/1000000000);
            
            // start = clock();
            start = CLOCK::now();

        }
        if(sz > c_controller -> high_threshold){
            Consumer *consumer = new Consumer(c_controller -> worker_queue, c_controller -> writer_queue, c_controller -> transformer);
            consumer -> start();
            c_controller -> consumers.push_back(consumer);
            printf("scaling up consumers from %d to %d\n", c_controller -> consumers.size()-1, c_controller -> consumers.size());

            // printf("size = %d\n", sz);
            // printf("scaling up consumers from %d to %d, %lf\n", c_controller -> consumers.size()-1, c_controller -> consumers.size(), (double)(std::chrono::duration_cast<std::chrono::nanoseconds>(CLOCK::now() - start).count())/1000000000);
            // start = clock();
            start = CLOCK::now();
        }
        usleep(c_controller -> check_period);
    }
    
    return nullptr;
}

#endif // CONSUMER_CONTROLLER_HPP
