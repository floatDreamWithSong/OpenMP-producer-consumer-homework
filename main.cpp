#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <omp.h>
#include "queue.h"


void tokenize(const std::string& line) {
    std::istringstream iss(line);
    std::string word;
    while (iss >> word) {
        std::cout <<"consumer "<< omp_get_thread_num()<< " : " << word << std::endl;
    }
}


void producer(const std::string& filename, SharedQueue& queue) {
    std::cout << "producer " << omp_get_thread_num() << " : " << filename << std::endl;
    std::ifstream file(filename);
    std::string line;
    
    if (file.is_open()) {
        while (std::getline(file, line)) {
            if (!line.empty()) {
                queue.push(line);
            }
        }
        file.close();
    } else {
        std::cerr << "无法打开文件: " << filename << std::endl;
    }
}


void consumer(SharedQueue& queue) {
    std::string line;
    while (!queue.isDone()) {
        if (queue.pop(line)) {
            tokenize(line);
        }
    }
}

int main() {
    const int n = 2; 
    std::vector<std::string> files = {
        "file1.txt",
        "file2.txt",
        "file3.txt",
        "file4.txt"
    };

    SharedQueue queue;
    omp_set_num_threads(2*n);
    #pragma omp parallel sections num_threads(2*n)
    {
        
        #pragma omp section
        {
            #pragma omp parallel for num_threads(n)
            for (int i = 0; i < files.size(); i++) {
                producer(files[i], queue);
            }
            queue.setDone();
        }

        
        #pragma omp section
        {
            #pragma omp parallel num_threads(n)
            {
                consumer(queue);
            }
        }
    }

    return 0;
}