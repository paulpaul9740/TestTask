﻿#include "Writer.h"
#include "FileReader.h"
#include "ThreadWorker.h"
#include <filesystem>
#include <iostream>
#include <thread>
#include <algorithm>
namespace fs = std::filesystem;
int main(int argc, char *argv[]) {
	if (argc != 3) {
		std::cout << "Usage: appname pathToInputDirs outputFile" << std::endl;
		exit(1);
	}
	std::string dirs{ argv[1] }, outFileName{ argv[2] };
	std::vector<std::thread> threadPool;
	constexpr unsigned int MIN_THREADS = 4;
	int threadNumber = std::max(std::thread::hardware_concurrency(),MIN_THREADS);
	std::queue<std::string> outQueue, taskQueue;
	std::mutex taskMutex, outMutex;
	std::condition_variable cond;
	std::cout << "Creating tasks...";
	for (const auto &entry : fs::directory_iterator(dirs)) {
		taskQueue.push(entry.path().string());
	}
	std::cout << "Done!" << std::endl;
	bool write = true;
	Writer writer{ outFileName, outQueue, outMutex, cond };
	std::thread writerThread{ &Writer::writeThread, &writer, std::ref(write) };
	ThreadWorker worker{ taskQueue, outQueue, taskMutex, outMutex, cond};
	std::cout << "Creating worker threads, threadNumber = " << threadNumber << std::endl;
	for (int i = 0 ; i < threadNumber; ++i) {
		threadPool.push_back(std::thread{ &ThreadWorker::start, &worker });
	}
	std::cout << "Workers are running..." << std::endl;
	for (auto it = threadPool.begin(); it != threadPool.end(); ++it) {
		it->join();
	}
	std::cout << "Workers stopped" << std::endl;
	write = false;
	cond.notify_one();
	writerThread.join();
	std::cout << "Writer stopped, check the " << outFileName << " for results." << std::endl;
	return 0;
}
