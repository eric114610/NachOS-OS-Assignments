#include <assert.h>
#include <stdlib.h>
#include "ts_queue.hpp"
#include "item.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "producer.hpp"
#include "consumer_controller.hpp"
#include "experiment.hpp"

#define READER_QUEUE_SIZE 200
#define WORKER_QUEUE_SIZE 200
#define WRITER_QUEUE_SIZE 4000
#define CONSUMER_CONTROLLER_LOW_THRESHOLD_PERCENTAGE 20
#define CONSUMER_CONTROLLER_HIGH_THRESHOLD_PERCENTAGE 80
#define CONSUMER_CONTROLLER_CHECK_PERIOD 1000000

int main(int argc, char** argv) {
    assert(argc == 4);
	// assert(argc == 5);
    // assert(argc == 6);

	int n = atoi(argv[1]);
	std::string input_file_name(argv[2]);
	std::string output_file_name(argv[3]);

    // experiment1
    // std::string exp_file("experiment/2/ccperiod1_" + (std::string)(argv[4]) + ".out");
    // int CONSUMER_CONTROLLER_CHECK_PERIOD = atoi(argv[4]);
    // experiment2
    // std::string exp_file("experiment/l_" + (std::string)(argv[4]) + "_h_" + (std::string)(argv[5]) + ".out");
    // int CONSUMER_CONTROLLER_LOW_THRESHOLD_PERCENTAGE = atoi(argv[4]);
    // int CONSUMER_CONTROLLER_HIGH_THRESHOLD_PERCENTAGE = atoi(argv[5]);
    // experiment3
    // std::string exp_file("experiment/worker_q_" + (std::string)(argv[4]) + ".out");
    // int WORKER_QUEUE_SIZE = atoi(argv[4]);
    // experiment4
    // std::string exp_file("experiment/write_q_" + (std::string)(argv[4]) + ".out");
    // int WRITER_QUEUE_SIZE = atoi(argv[4]);
    // experiment5
    // std::string exp_file("experiment/read_q_" + (std::string)(argv[4]) + ".out");
    // int READER_QUEUE_SIZE = atoi(argv[4]);

	// TODO: implements main function

    TSQueue<Item*> input_queue(READER_QUEUE_SIZE), output_queue(WRITER_QUEUE_SIZE), worker_queue(WORKER_QUEUE_SIZE);
    Transformer transfomrer;

    Reader reader(n, input_file_name, &input_queue);
    Producer *producer[4];
    for(int i = 0 ; i < 4; i++){
        producer[i] = new Producer(&input_queue, &worker_queue, &transfomrer);
    }
    ConsumerController *c_contorller = new ConsumerController(&worker_queue, &output_queue, &transfomrer, CONSUMER_CONTROLLER_CHECK_PERIOD, WORKER_QUEUE_SIZE/100*CONSUMER_CONTROLLER_LOW_THRESHOLD_PERCENTAGE, WORKER_QUEUE_SIZE/100*CONSUMER_CONTROLLER_HIGH_THRESHOLD_PERCENTAGE);
    Writer writer(n, output_file_name, &output_queue);

    // Experiment exp(exp_file, &worker_queue, c_contorller, 100000); //experiment
    // exp.start();

    reader.start();
    for(int i = 0 ; i < 4; i++){
        producer[i] -> start();
    }
    c_contorller -> start();
    writer.start();
    
    reader.join();
    writer.join();

	return 0;
}
