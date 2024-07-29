#include <string>
#include <sstream>
#include <iostream>
#include <thread>
#include <functional>
#include <vector>

#include "PCB.h"
#include "disk.h"
#include "OS.h"

using namespace std;

void runSimulation(OperatingSystem &OS)
{
    while (OS.isRunning())
    {

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        OS.runCPU(1);
        OS.Snapshot();
    }
}
void sortVector(vector<vector<int>> &process)
{
}

void addProcess(OperatingSystem &OS, vector<vector<int>> &processes)
{
    int time = 0;

    while (processes.size())
    {

        vector<vector<vector<int>>::iterator> temp;

        for (auto itr = processes.begin(); itr != processes.end(); itr++)
        {
            if ((*itr)[0] == time)
            {

                OS.CreateProcess((*itr)[0], (*itr)[1], (*itr)[2], (*itr)[3], (*itr)[4]);
                temp.push_back(itr);
            }
        }

        for (int i = temp.size() - 1; i >= 0; i--)
        {
            processes.erase(temp[i]);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        time++;
    }
}

int main()
{

    unsigned int RAM = 0;
    unsigned int page_size = 0;
    int number_of_hard_disks = 0;
    int quantumTime = 0;
    int quantum = 5;
    int quantumReset = 0;
    int page_replacement_algo = 0;
    int simulationType;

    std::cout << "How much RAM is there?" << std::endl;
    std::cin >> RAM;

    std::cout << "What is the size of a page/frame?" << std::endl;
    std::cin >> page_size;

    std::cout << "How many hard disks does the simulated computer have?" << endl;
    std::cin >> number_of_hard_disks;

    OperatingSystem OS(number_of_hard_disks, RAM, page_size);

    std::cout << "What do you want to simulate: \n0 -> MLFQ Scheduling \n1 -> Page Replacement Algorithms \nInput (0,1): ";
    std::cin >> simulationType;

    if (simulationType == 1)
    {
        std::cout << "Choose the page replacement algorithm (Enter the corresponding number)?" << std::endl;
        std::cout << "0 -> Lease Recently Used (LRU) (default)" << std::endl;
        std::cout << "1 -> Lease Frequently Used (LFU)" << std::endl;
        std::cout << "2 -> Random replacement" << std::endl;
        std::cout << "3 -> First In First Out (FIFO)" << std::endl;
        std::cout << "4 -> Most Recently Used (MRU)" << std::endl;
        std::cin >> page_replacement_algo;

        if (page_replacement_algo < 0 || page_replacement_algo > 4)
            page_replacement_algo = 0;

        OperatingSystem OS(number_of_hard_disks, RAM, page_size, page_replacement_algo);

        std::string input;
        std::getline(std::cin, input);

        while (1)
        {
            // Shows which process is currently using the CPU and which processes are waiting in the ready-queue.
            if (input == "S r")
            {
                OS.Snapshot();
            }
            // Shows which processes are currently using the hard disks and which processes are waiting to use them
            else if (input == "S i")
            {
                OS.IOSnapshot();
            }
            // Shows the state of memory.
            else if (input == "S m")
            {
                OS.MemorySnapshot();
            }
            // Creates a new pcb and places it at end of ready queue, or in the CPU if the ready queue is empty.
            else if (input == "A")
            {
                OS.CreateProcess();
            }
            // The currently running process has spent a time quantum using the CPU.
            else if (input == "Q")
            {
                OS.CPUToReadyQueue();
            }
            // The process using the CPU forks a child.The child is placed in the end of the ready - queue.
            else if (input == "fork")
            {
                OS.Fork();
            }
            // The process that is currently using the CPU terminates.
            else if (input == "exit")
            {
                OS.Exit();
            }
            // The process wants to pause and wait for any of its child processes to terminate.
            else if (input == "wait")
            {
                OS.Wait();
            }

            // For other commands, parse input
            std::stringstream in_stream(input);
            string first_word;
            in_stream >> first_word;

            if (first_word == "d" || first_word == "D" || first_word == "m")
            {
                int second_word;
                in_stream >> second_word;
                // The process that currently uses the CPU requests the hard disk #number.
                // It wants to read or write file file _name.
                if (first_word == "d")
                {
                    string third_word;
                    in_stream >> third_word;
                    OS.RequestDisk(second_word, third_word);
                }
                else if (first_word == "D")
                { // "D number") {
                    // The hard disk #number has finished the work for one process.
                    OS.RemoveProcessFromDisk(second_word);
                }
                // The process that is currently using the CPU requests a memory operation for the logical address.
                else if (first_word == "m")
                { // == "m address") {
                    OS.RequestMemoryOperation(second_word);
                }
            }
            // Get next line of input from user
            std::cout << endl;
            std::getline(std::cin, input);
        }

        delete &OS;
    }
    else
    {
        int number_of_processes;
        cout << "Total number of processes that will be running on system: ";
        cin >> number_of_processes;

        vector<vector<int>> processes;
        int burst_time, start_time, memory, priority, processNumber;

        cout << "\nEnter: [Arrival Time] [Burst Time] [Priority] [Memory] [Process Number]\n";
        for (int i = 0; i < number_of_processes; i++)
        {
            cin >> start_time >> burst_time >> priority >> memory >> processNumber;
            vector<int> process;
            process.push_back(start_time);
            process.push_back(burst_time);
            process.push_back(priority);
            process.push_back(memory);
            process.push_back(processNumber);
            processes.push_back(process);
        }

        // Printing processes
        // for (int i =0; i < processes.size(); i++){
        //     cout << "\nProcess: ";
        //     for (int j =0; j < processes[i].size(); j ++){
        //         cout << processes[i][j] << " ";
        //     }
        // }
        cout << "\n# Starting Simulation.......\n";
        std::thread addProcessThread(addProcess, std::ref(OS), std::ref(processes));
        // std::thread simulationThread(runSimulation, std::ref(OS), std::ref(quantum) , std::ref(quantumTime));
        std::thread simulationThread(runSimulation, std::ref(OS));

        addProcessThread.join();
        cout << "\n# All processes successully added to ready queues for execution #\n# Enter 'E' (exit), 'A' (add) or 'T' (time):\n";

        char ch;
        cin >> ch;
        while (1)
        {
            if (ch == 'T')
            {
                OS.printWaitingTimes();
            }
            else if (ch == 'A')
            {
                cin >> burst_time >> priority >> memory >> processNumber;

                OS.CreateProcess(-1, burst_time, priority, memory, processNumber);
            }
            else if (ch == 'E')
            {

                break;
            }
            else
            {
                cout << "Wrong Character!!!! # Enter Q, T or E :\n";
            }

            cin >> ch;
        }
        cout << "Character entered : " << ch;

        OS.Exit();
        simulationThread.join();
    }

    // while (1) {

    //     OS.CreateProcess();
    // }
    // char arg;

    // while (1){
    //     cin >> arg;
    //     if (arg == 'A'){
    //         cin >> start_time >> burst_time >> priority >> memory;
    //         // cout << arg << start_time << burst_time << priority << memory;
    //         OS.CreateProcess(start_time, burst_time, priority, memory);

    //     }
    // }

    // std::string input;
    // std::getline(std::cin, input);

    // while (1) {
    //      //Shows which process is currently using the CPU and which processes are waiting in the ready-queue.
    //     if (input == "S r") {
    //         OS.Snapshot();
    //     }
    //     //Shows which processes are currently using the hard disks and which processes are waiting to use them
    //     else if (input == "S i") {
    //         OS.IOSnapshot();
    //     }
    //     //Shows the state of memory.
    //     else if (input == "S m") {
    //         OS.MemorySnapshot();
    //     }
    //     // Creates a new pcb and places it at end of ready queue, or in the CPU if the ready queue is empty.
    //     else if (input == "A") {
    //         OS.CreateProcess();
    //     }
    //     //The currently running process has spent a time quantum using the CPU.
    //     else if (input == "Q") {
    //         OS.CPUToReadyQueue();
    //     }
    //     //The process using the CPU forks a child.The child is placed in the end of the ready - queue.
    //     else if (input == "fork") {
    //         OS.Fork();
    //     }
    //     //The process that is currently using the CPU terminates.
    //     else if (input == "exit") {
    //         OS.Exit();
    //     }
    //     //The process wants to pause and wait for any of its child processes to terminate.
    //     else if (input == "wait") {
    //         OS.Wait();
    //     }

    //     // For other commands, parse input
    //     std::stringstream in_stream(input);
    //     string first_word;
    //     in_stream >> first_word;

    //     if (first_word == "d" || first_word == "D" || first_word == "m") {
    //         int second_word;
    //         in_stream >> second_word;
    //         //The process that currently uses the CPU requests the hard disk #number.
    //         //It wants to read or write file file _name.
    //         if (first_word == "d") {
    //             string third_word;
    //             in_stream >> third_word;
    //             OS.RequestDisk(second_word, third_word);
    //         }
    //         else if (first_word == "D") { // "D number") {
    //             // The hard disk #number has finished the work for one process.
    //             OS.RemoveProcessFromDisk(second_word);
    //         }
    //         //The process that is currently using the CPU requests a memory operation for the logical address.
    //         else if (first_word == "m") { // == "m address") {
    //             OS.RequestMemoryOperation(second_word);
    //         }
    //     }
    //     // Get next line of input from user
    //     std::cout << endl;
    //     std::getline(std::cin, input);
    // }
}