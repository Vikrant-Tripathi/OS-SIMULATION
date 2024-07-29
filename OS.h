#ifndef OS_H
#define OS_H

#include "PCB.h"
#include "disk.h"

#include <algorithm>
#include <iostream>
#include <vector>
#include <list>
#include <map>
#include <cmath>
#include <queue>

int power (int x, int p) {
  int i = 1;
  for (int j = 1; j <= p; j++)  i *= x;
  return i;
}

class Timing {
    public:
        Timing(int processNumber_, int arrival_time_, int finish_time_, int cpu_burst_) : processNumber(processNumber_), arrival_time(arrival_time_), finish_time(finish_time_), cpu_burst(cpu_burst_) {}

        int processNumber;
        int arrival_time;
        int finish_time;
        int cpu_burst;

};

class OperatingSystem {
    public:
        OperatingSystem(int & number_of_hard_disks_, unsigned int & RAM_, unsigned int & page_size_, int page_replacement_algo_=0) : 
            CPU(1), 
            number_of_processes(1), 
            timestamp(0), 
            number_of_hard_disks(number_of_hard_disks_), 
            page_size(page_size_), 
            RAM(RAM_),
            number_of_frames(RAM_ / page_size_), 
            current_time(0),
            total_time(0),
            current_quantum(1e6),
            is_running(true),
            // ready_queue(0), 
            hard_disks(number_of_hard_disks_), 
            page_replacement_algo(page_replacement_algo_),

            frames(0) {

            srand(time(0));
                
            // Creates initial process
            PCB* process_1 = new PCB{ number_of_processes , 0, 500, 4, 0, 1};
            process_1->SetParent(1); //First process has no parent
            all_processes[number_of_processes] = process_1;

            std::list <int> * q1 = new std::list <int>(); 
            std::list <int> * q2 = new std::list <int>(); 
            std::list <int> * q3 = new std::list <int>(); 
            std::list <int> * q4 = new std::list <int>(); 
            ready_queues.push_back(q1);
            ready_queues.push_back(q2);
            ready_queues.push_back(q3);
            ready_queues.push_back(q4);


            // Initialize hard disks
            for (int i = 0; i < number_of_hard_disks; i++) {
                HardDisk* disk_ = new HardDisk{};
                hard_disks[i] = disk_;
            }
        }

        ~OperatingSystem() {
            // Delete disks
            for (auto disk : hard_disks) {
                delete disk;
            }
            // Delete frames
            for (auto frame : frames) {
                delete frame;
            }
            // Delete all PCBs
            for (auto PCB : all_processes) {
                delete PCB.second;
            }
            while (!fifo.empty()){
                Pair* pid_page = fifo.front();
                fifo.pop();
                delete pid_page;
            }
            


            hard_disks.clear();
            frames.clear();
            all_processes.clear();
            // ready_queues.clear();
        }

        void runCPU(int time_unit_){
            current_time += time_unit_;
            total_time += time_unit_;

            PCB * pcb = all_processes[CPU];
            // int current_burst = pcb->getBurstTime();
            pcb->setBurstTime(pcb->getBurstTime() - 1);

            if (pcb->getBurstTime() <= 0) {
                Exit();
                // time = 0;
            } else if (current_time >= current_quantum){
                if (pcb->getPriority() < ready_queues.size() - 1){
                    pcb->setPriority(pcb->getPriority() + 1);
                }
                
                CPUToReadyQueue();
                // time = 0;
            }

        }
        


        // Creates a new process and adds it to the ready queue, or the CPU if it is empty.
        // The parent of the new process is process 1.
        void CreateProcess(int arrival_time = -1 , int burst_time = 20, int priority = 0, int memory = 0, int processNumber = -1) {

            if (arrival_time == -1) {
                arrival_time = total_time;
            }

            PCB* new_process = new PCB{ ++number_of_processes , arrival_time, burst_time, priority, memory, processNumber};
            all_processes[number_of_processes] = new_process;
            
            // Parent process is 1
            all_processes[1]->AddChildProcess(new_process);
            new_process->SetParent(1);

            AddToReadyQueue(number_of_processes, priority);

            if (priority < all_processes[CPU]->getPriority()){
                CPUToReadyQueue();
                // time = 0;
            } 

            // Adds new process to ready queue
            
        }

        
        // Creates a new process whose parent is the pcb currently using CPU.
        void Fork(int start_time = -1, int burst_time = 20, int priority = 0, int memory = 0) {
            if (CPU == 1) { // If fork called with no process in CPU
                std::cout << "There is no process in the CPU to fork" << std::endl;
            }
            else {
                
                PCB* new_process = new PCB(++number_of_processes, start_time,burst_time,priority,memory, -1);

                // Parent process is the process in the CPU that called fork
                all_processes[CPU]->AddChildProcess(new_process); 
                new_process->SetParent(CPU);

                // Add new process to ready queue
                AddToReadyQueue(number_of_processes, priority);

                all_processes[number_of_processes] = new_process;
            }
        }

        // Adds the given process to the ready queue, or if the CPU is idle, the process goes straight there instead.
        void AddToReadyQueue(const int pid, int priority) {
            // First process goes straight to CPU
            if (CPU == 1) {
                CPU = pid;
                if (priority < ready_queues.size() -1 ){
                    current_quantum = 4*power(2,priority);
                } else {
                    current_quantum = 1e6;
                }
                
            }
            // Otherwise process added to back of queue
            else {
                ready_queues[priority]->push_back(pid);
            }
        }

        // Shows the process currently using the CPU, and lists any processes in the ready queue.
        void Snapshot()  {
            std::cout << "\nTime: "<< total_time << "\nProcess using CPU: " << all_processes[CPU]->getProcessNumber() << " " << all_processes[CPU]->getPriority() << " " << (all_processes[CPU])->getBurstTime()<< std::endl;
            std::cout << "Ready Queues:  \n";
            for (int i = 0; i < ready_queues.size() ; i++) {
                if (i == ready_queues.size() - 1) {
                    std::cout << "Queue " << i << " (FCFS)" <<":";
                } else {
                    std::cout << "Queue " << i << " (Quantum " << 4*power(2,i) << " )" <<":";
                }
                for (auto itr1 = ready_queues[i]->begin(); itr1 != ready_queues[i]->end(); ++itr1){

                    std::cout <<  " <- " << all_processes[*itr1]->getProcessNumber() << " " << all_processes[*itr1]->getPriority() << " " << (all_processes[*itr1])->getBurstTime()  ;
                }
                std::cout << std::endl;
            }
            
            
        }


        // Moves the process currently running in CPU to the end of the ready queue
        // Also places a new process into the CPU, if there is one in the ready queue
        void CPUToReadyQueue() {    
            int priority = all_processes[CPU]->getPriority();
            // if (priority < ready_queues.size() -  1){

            //     all_processes[CPU]->setPriority(priority+1);
            //     ready_queues[priority+1]->push_back(CPU);
                
            // } else {
                
            // }
            ready_queues[priority]->push_back(CPU);

            for (int i = 0; i < ready_queues.size() ; i++){
                if (! ready_queues[i]->empty()){
                    CPU = ready_queues[i]->front();
                    current_quantum = 4*power(2,i);
                    ready_queues[i]->pop_front();
                    break;
                }
            }
            current_time =0;
            
        }

        // The process using the CPU calls wait.
        void Wait() {
            PCB* cpu_process = all_processes[CPU];

            // If the process has no children, nothing to wait for
            if (cpu_process->HasChildren()) {
                // If the process already has at least one zombie child, child disappears and parent keeps using CPU
                int PID_of_zombie_child = cpu_process->ProcessHasZombieChild();
                if (PID_of_zombie_child > 0) {  ///////////???
                    cpu_process->SetWaitingState(0);

                    // Delete the zombie child's children
                    DeleteChildren(all_processes[PID_of_zombie_child]);
                    
                    // Delete the zombie child
                    PCB* zombie = all_processes[PID_of_zombie_child];
                    PCB* parent = all_processes[zombie->GetParent()];

                    parent->RemoveChild(zombie);
                    all_processes.erase(PID_of_zombie_child);
                    delete zombie;
                    zombie = nullptr;
                }

                // If the process has no zombie children, remove the process from the ready queue and send process at front of ready queue to CPU
                else {
                    // Set process using CPU to waiting
                    cpu_process->SetWaitingState(1);

                    for (int i =0; i < ready_queues.size() ; i++){
                        if (ready_queues[i]->empty()) {
                            if (i == ready_queues.size()){
                                CPU = 1;
                            } 
                            continue;
                        } else {
                            CPU = ready_queues[i]->front();
                            ready_queues[i]->pop_front();
                        }
                    }

                    // The waiting parent process will be added back to the end of the ready queue when one of its children exits.
                }
            }
        }

		//The process that is currently using the CPU terminates. If its parent is already waiting, the process terminates immediately and the parent 
		// goes to the end of the ready queue. If the parent isn't waiting, the process becomes a zombie process. If the parent is process 1, the 
		// process terminates immediately. All children of the process are terminated.
        void Exit() {
            PCB* exiting_process = all_processes[CPU];
            if (CPU == 1){
                is_running = false;
            }
            PCB* parent = all_processes[exiting_process->GetParent()];

            process_timings[exiting_process->getProcessNumber()] = new Timing(exiting_process->getProcessNumber(),exiting_process->getArrivalTime(), total_time, exiting_process->getTotalCPUBurst());

            //If parent is waiting, the process (and all of its children) terminate immediately, and the parent goes to the end of the ready queue.
            if (parent->IsWaiting()) {
                // Terminate all children and the process
                DeleteChildren(exiting_process);
                all_processes.erase(exiting_process->GetPid());
                parent->RemoveChild(exiting_process);
                delete exiting_process;
                exiting_process = nullptr;

                // Parent goes to the end of the ready queue and is no longer waiting, and the process exits the CPU
                parent->SetWaitingState(0);
                AddToReadyQueue(parent->GetPid(), parent->getPriority());
                GetNextFromReadyQueue();
            }
            //If parent is process 1, terminate the process and all children immediately.
            else if (parent->GetPid() == 1) {
                //Terminate children
                DeleteChildren(exiting_process);

                //Terminate process
                all_processes.erase(exiting_process->GetPid());
                parent->RemoveChild(exiting_process);
                delete exiting_process;
                exiting_process = nullptr;

                // Replace the process in the CPU
                GetNextFromReadyQueue();

            }
            // Parent is not waiting, so process becomes a zombie.
            else {
                // Mark process as a zombie
                exiting_process->SetZombie(1);

                // Terminate all children
                DeleteChildren(exiting_process);
                GetNextFromReadyQueue();
            }
            // time = 0;
        }

        // Removes a process from the ready queue, if the ready queue contains the process
        void RemoveFromReadyQueue(const int pid_) {

            for (int i = 0; i < ready_queues.size() ; i++){
                auto pid_to_delete = std::find(ready_queues[i]->begin(), ready_queues[i]->end(), pid_);
                // If the item is in the ready queue, removes it
                if (pid_to_delete != ready_queues[i]->end()) {
                    ready_queues[i]->erase(pid_to_delete);
                }

            }
            
            
        }

        //Checks each hard disk for the given process. If the process is using any of the disks or is on one of their io queues, it is removed.
        void RemoveFromDisks(const int & pid_) {
            for (auto itr = hard_disks.begin(); itr != hard_disks.end(); itr++) {
                (*itr)->Remove(pid_);
            }
        }


        //Shows the state of memory.
        //For each used frame, displays the process number that occupies it and the page number stored in it.
        //The enumeration of pages and frames starts from 0.
        void MemorySnapshot() {
            if(page_replacement_algo == 1){
                std::cout << "Frame   " << "Page Number     " << "pid       " << "freq" << std::endl;
            }
            else if(page_replacement_algo == 2){
                std::cout << "Frame   " << "Page Number     " << "pid       " << std::endl;
            }
            else if(page_replacement_algo == 3){
                std::cout << "Frame   " << "Page Number     " << "pid       " << std::endl;
            }
            else if(page_replacement_algo == 4){
                std::cout << "Frame   " << "Page Number     " << "pid       " << "ts" << std::endl;
            }
            else{
                std::cout << "Frame   " << "Page Number     " << "pid       " << "ts" << std::endl;
            }

            for (unsigned int i = 0; i < frames.size(); i++) {
                std::cout << "  " <<  i << "        ";
                if (frames[i]->pid_ != 0) {
                    if(page_replacement_algo == 1){
                        std::cout << "   " << frames[i]->page_ << "          " << frames[i]->pid_ << "         " << frames[i]->frequency_;
                    }
                    else if(page_replacement_algo == 2){
                        std::cout << "   " << frames[i]->page_ << "          " << frames[i]->pid_;
                    }
                    else if(page_replacement_algo == 3){
                        std::cout << "   " << frames[i]->page_ << "          " << frames[i]->pid_;
                    }
                    else if(page_replacement_algo == 4){
                        std::cout << "   " << frames[i]->page_ << "          " << frames[i]->pid_ << "         " << frames[i]->timestamp_;
                    }
                    else{
                        std::cout << "   " << frames[i]->page_ << "          " << frames[i]->pid_ << "         " << frames[i]->timestamp_;
                    }
                }
                std::cout << std::endl;
            }
        }

        //The process that is currently using the CPU requests a memory operation for the logical address.
        void RequestMemoryOperation(const int & address) {
            int page = address / page_size;
            
            if(page_replacement_algo == 1){
                int least_frequent_count = INT_MAX, least_frequent_idx = -1;

                for (unsigned int i = 0; i < frames.size(); i++) {
                    if ((frames[i]->page_ == page) && (frames[i]->pid_ == CPU)) {
                        frames[i]->frequency_++;
                        return;
                    }
                }

                if (frames.size() < number_of_frames) {
                    Frame* new_frame = new Frame{};
                    new_frame->frequency_ = 1;
                    new_frame->pid_ = CPU;
                    new_frame->page_ = page;
                    frames.push_back(new_frame);
                }
                else {
                    for (unsigned int i = 0; i < frames.size(); i++) {
                        if (frames[i]->frequency_ <= least_frequent_count) {
                            least_frequent_idx = i;
                            least_frequent_count = frames[i]->frequency_;
                        }
                    }
                    frames[least_frequent_idx]->page_ = page;
                    frames[least_frequent_idx]->pid_ = CPU;
                    frames[least_frequent_idx]->frequency_ = 1;
                }
            }
            else if(page_replacement_algo == 2){
                for (unsigned int i = 0; i < frames.size(); i++) {
                    if ((frames[i]->page_ == page) && (frames[i]->pid_ == CPU)) {
                        return;
                    }
                }

                if (frames.size() < number_of_frames) {
                    Frame* new_frame = new Frame{};
                    new_frame->pid_ = CPU;
                    new_frame->page_ = page;
                    frames.push_back(new_frame);
                }
                else{
                    int random_frame = int((((double)rand()) / RAND_MAX) * frames.size());
                    // std::cout << "random frame " << random_frame << std::endl;

                    frames[random_frame]->page_ = page;
                    frames[random_frame]->pid_ = CPU;
                }
            }
            else if(page_replacement_algo == 3){
                for (unsigned int i = 0; i < frames.size(); i++) {
                    if ((frames[i]->page_ == page) && (frames[i]->pid_ == CPU)) {
                        return;
                    }
                }

                if (frames.size() < number_of_frames) {
                    Frame* new_frame = new Frame{};
                    new_frame->pid_ = CPU;
                    new_frame->page_ = page;
                    frames.push_back(new_frame);
                    Pair* CPU_page = new Pair();
                    CPU_page -> first = CPU;
                    CPU_page -> second = page;
                    fifo.push(CPU_page);
                }
                else{
                    Pair* pid_pageNum = fifo.front();
                    fifo.pop();
                    int replacedFrame = 0;

                    for (unsigned int i = 0; i < frames.size(); i++) {
                        if (frames[i]->pid_ == pid_pageNum -> first && frames[i]->page_ == pid_pageNum -> second) {
                            replacedFrame = i;
                            break;
                        }
                    }

                    frames[replacedFrame]->page_ = page;
                    frames[replacedFrame]->pid_ = CPU;
                    Pair* CPU_page = new Pair();
                    CPU_page -> first = CPU;
                    CPU_page -> second = page;
                    fifo.push(CPU_page);
                    delete pid_pageNum;
                }
            }
            else if(page_replacement_algo == 4){
                int newest_timestamp = 0;
                int index_of_newest = -1;

                // Adds frames to the vector as needed until the vector is full (size is number_of_frames). Then replaces the least recently used frame
                // If the same process wants to access the same page, just update time stamp
                for (unsigned int i = 0; i < frames.size(); i++) {
                    if ((frames[i]->page_ == page) && (frames[i]->pid_ == CPU)) {
                        frames[i]->timestamp_ = timestamp;
                        timestamp++;
                        return;
                    }
                }

                // If there are empty frames, create a new frame in the vector
                if (frames.size() < number_of_frames) {
                    Frame* new_frame = new Frame{};
                    new_frame->timestamp_ = timestamp;
                    new_frame->pid_ = CPU;
                    new_frame->page_ = page;
                    frames.push_back(new_frame);
                }

                // Replace the least recently used frame's data
                else {
                    // Find the frame with the oldest(lowest) timestamp
                    for (unsigned int i = 0; i < frames.size(); i++) {
                        if (frames[i]->timestamp_ >= newest_timestamp) {
                            index_of_newest = i;
                            newest_timestamp = frames[i]->timestamp_;
                        }
                    }
                    frames[index_of_newest]->page_ = page;
                    frames[index_of_newest]->pid_ = CPU;
                    frames[index_of_newest]->timestamp_ = timestamp;
                }
                timestamp++;
            }
            else{
                int oldest_timestamp = timestamp;
                int index_of_oldest = -1;

                // Adds frames to the vector as needed until the vector is full (size is number_of_frames). Then replaces the least recently used frame
                // If the same process wants to access the same page, just update time stamp
                for (unsigned int i = 0; i < frames.size(); i++) {
                    if ((frames[i]->page_ == page) && (frames[i]->pid_ == CPU)) {
                        frames[i]->timestamp_ = timestamp;
                        timestamp++;
                        return;
                    }
                }

                // If there are empty frames, create a new frame in the vector
                if (frames.size() < number_of_frames) {
                    Frame* new_frame = new Frame{};
                    new_frame->timestamp_ = timestamp;
                    new_frame->pid_ = CPU;
                    new_frame->page_ = page;
                    frames.push_back(new_frame);
                }

                // Replace the least recently used frame's data
                else {
                    // Find the frame with the oldest(lowest) timestamp
                    for (unsigned int i = 0; i < frames.size(); i++) {
                        if (frames[i]->timestamp_ <= oldest_timestamp) {
                            index_of_oldest = i;
                            oldest_timestamp = frames[i]->timestamp_;
                        }
                    }
                    frames[index_of_oldest]->page_ = page;
                    frames[index_of_oldest]->pid_ = CPU;
                    frames[index_of_oldest]->timestamp_ = timestamp;
                }
                timestamp++;
            }
        }

        // Shows which processes are currently using the hard disks and what processes are waiting to use them.
        void IOSnapshot() const {
            for (int i = 0; i < number_of_hard_disks; i++) {
                std::cout << "Disk " << i << ": ";
                if (hard_disks[i]->DiskIsIdle()) {
                    std::cout << "idle" << std::endl;
                }
                else {
                    std::cout << "[" << hard_disks[i]->GetCurrentProcess() << " " << hard_disks[i]->GetCurrentFile() << "]" << std::endl;
                    std::cout << "Queue for disk " << i << ": ";
                    hard_disks[i]->PrintQueue();
                }
            }
        }

        // The process using the CPU requests the hard disk disk_number
        // It wants to read or write file file _name.
        void RequestDisk(const int & disk_number, const std::string & file_name) {
            if ((disk_number < number_of_hard_disks) && (disk_number >= 0)) {
                // Process 1 should not use any disks/be added to any queues
                if (CPU != 1) {
                    hard_disks[disk_number]->Request(file_name, CPU);
                    // Remove from CPU and replace from ready queue
                    GetNextFromReadyQueue();
                }
            }
            else {
                std::cout << "There is no disk " << disk_number << std::endl;
            }
        }

        // The process at the front of the ready queue is removed and moves to the CPU.
        void GetNextFromReadyQueue() {
            for (int i =0 ; i < ready_queues.size(); i++){
                if (ready_queues[i]->empty()) {
                    if (i == ready_queues.size() - 1){
                        CPU = 1;
                        current_quantum = 1e6;

                    } 
                    continue;
                }
                else {
                    CPU = ready_queues[i]->front();
                    ready_queues[i]->pop_front();
                    current_quantum = 4* power(2,i);
                    break;
                }
            }
            current_time = 0;
            
        }

        // When a process is done using a disk, puts it back onto the ready queue.
        void RemoveProcessFromDisk(const int & disk_number) {
            if ((disk_number < number_of_hard_disks) && (disk_number >= 0)) {
                int removed_pcb = hard_disks[disk_number]->RemoveProcess();
                
                if(removed_pcb != -1){
                    AddToReadyQueue(removed_pcb, all_processes[removed_pcb]->getPriority());
                }
            }
            else {
                std::cout << "There is no disk " << disk_number << std::endl;
            }
        }

        //Checks each frame for the given process. If the process is found it is removed.
        void RemoveFromFrames(const int & pid) {
            for (auto itr = frames.begin(); itr != frames.end(); itr++) {
                if ((*itr)->pid_ == pid) {
                    (*itr)->Clear();
                }
            }
        }

        int getTotalTime () {
            return total_time;
        }

        void printWaitingTimes(){
            std::cout << "\nWaiting Times: \n";
            int total_waiting_time = 0;

            for (auto itr = process_timings.begin(); itr != process_timings.end(); itr++){
                int waiting_time = itr->second->finish_time - itr->second->arrival_time - itr->second->cpu_burst;

                std::cout << "Process " << itr->first << ": " << itr->second->arrival_time << " -> " << itr->second->finish_time << " Waiting Time: " << waiting_time << std::endl;

                total_waiting_time += waiting_time;
            }
            if (!process_timings.size()){
                std::cout << "Average Waiting Time: "<< 0 << std::endl;;
            } else {
                std::cout << "Average Waiting Time: " << total_waiting_time / process_timings.size() << std::endl;
            }
            
        }
        bool isRunning (){
            return is_running;
        }

    private:
        int CPU;   								// The pid of the process currently using the CPU
        int number_of_processes;    			// Not the current number of processes, but keeps track of how many are created while the program runs.
        int timestamp;							// For keeping track of memory requests
        int current_time;
        int current_quantum;
        int total_time;

        const int page_replacement_algo;        // Check the page replacement algorithm

        bool is_running;
        const int number_of_hard_disks;      	
        const unsigned int page_size;
        const unsigned int RAM;
        const unsigned int number_of_frames;     

        std::map<int, Timing*> process_timings;  
        std::vector<std::list<int>*> ready_queues;				// Holds the pids of processes waiting on the ready queue
        std::vector<HardDisk*> hard_disks; 		// Index of the vector is the disk number (disk 0 to disk n), holding a pointer to that disk
        std::map<int, PCB*> all_processes;   	// A map of all processes; The key is the pid of the process, the value is the pointer to that process
        
        struct Pair {
            int first;
            int second;

            Pair() : first(1), second(0) {};
            ~Pair() {};
        };

        std::queue<Pair*> fifo;

        struct Frame {
            int timestamp_;
            int page_;
            int pid_;
            int frequency_;

            Frame() : timestamp_(0), page_(0), pid_(0), frequency_(0) {}
            ~Frame() {}

            bool IsEmpty() {
                return pid_ == 0;
            }
            
            void Clear() {
                timestamp_ = 0;
                page_ = 0;
                pid_ = 0;
                frequency_ = 0;
            }
        };

        std::vector<Frame*> frames;

        // Deletes all children of a process, and removes them and the process pcb from all disks, frames, their queues and the ready queue.
        void DeleteChildren(PCB* pcb) {
            //Delete all children of the process pcb_to_delete
            for (auto itr = pcb->GetChildren().begin(); itr != pcb->GetChildren().end(); itr++) {
                PCB* child = (*itr);
                if (child->HasChildren()) {
                    DeleteChildren(child);
                }
                else {
                    RemoveFromDisks(child->GetPid());
                    RemoveFromFrames(child->GetPid());
                    RemoveFromReadyQueue(child->GetPid());

                    // Delete child from all_processes
                    all_processes.erase(child->GetPid());
                    delete child;
                    child = nullptr;
                }
            }

            pcb->ClearChildren();

            // Keep zombies around and delete later
            if (!pcb->IsZombieProcess()) {
                all_processes.erase(pcb->GetPid());
            }

            RemoveFromDisks(pcb->GetPid());
            RemoveFromFrames(pcb->GetPid());
            RemoveFromReadyQueue(pcb->GetPid());
        }
};


#endif //OS_H
