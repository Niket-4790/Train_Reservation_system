#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
using namespace std;
using namespace std::chrono;

#define MAX_TRAINS 100
#define CAPACITY 500
#define BOOK_MIN 5
#define BOOK_MAX 10
#define MAX_THREADS 20
#define MAX_ACTIVE_QUERIES 5
#define MAX_TIME 1 // mins

std::mutex train_num[MAX_TRAINS];
std::mutex query_mutex;
std::condition_variable query_cond;

struct query {
    int type;
    int train_num;
    int thread_num;
};

struct query active_queries[MAX_ACTIVE_QUERIES];
int num_active_queries = 0;

int available_seats[MAX_TRAINS];
std::thread threads[MAX_THREADS];
int num_threads = 0;
std::mutex print_mutex;

int get_random_train() {
    return std::rand() % MAX_TRAINS;
}

int get_random_bookings() {
    return std::rand() % (BOOK_MAX - BOOK_MIN + 1) + BOOK_MIN;
}

int get_random_thread() {
    return std::rand() % num_threads;
}

void print_query(struct query q) {
    // print_mutex.lock();
    cout << "Thread " << q.thread_num << ": ";
    if (q.type == 1) {
        cout << "Inquiring " << q.train_num << std::endl;
    } else if (q.type == 2) {
        cout << "Booking seats in " << q.train_num << std::endl;
    } else if (q.type == 3) {
        cout << "Cancelling bookings in " << q.train_num << std::endl;
    } else {
        cout << "Invalid query type" << std::endl;
    }
    // print_mutex.unlock();
}

void worker_thread(int thread_num) {
    auto start = std::chrono::steady_clock::now();
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() % 10000));
        struct query q;
        q.thread_num = thread_num;
        int train_num = get_random_train();
        q.train_num = train_num;
        int type = std::rand() % 3 + 1;
        q.type = type;
        // print_query(q);
        std::unique_lock<std::mutex> lock(query_mutex);
        while (num_active_queries >= MAX_ACTIVE_QUERIES) {
            query_cond.wait(lock);
        }
        bool conflict = false;
        for (int i = 0; i < num_active_queries; i++) {
            struct query active_query = active_queries[i];
            if (active_query.train_num == train_num) {
                if (active_query.type == 2 || type == 2) {
                    conflict = true;
                }
            }
        }
        if (conflict) {
            query_cond.wait(lock);
        }
        active_queries[num_active_queries++] = q;
        lock.unlock();
        //print here
        print_mutex.lock();
        print_query(q);
        switch (type) {
            case 1:
                cout << "Thread " << thread_num << ": " << available_seats[train_num] << " seats available in " << train_num << std::endl;
                // print_mutex.unlock();
                break;
            case 2:
                if (available_seats[train_num] >= BOOK_MIN) {
                    int num_booked = get_random_bookings();
                    if (num_booked > available_seats[train_num]) {
                        num_booked = available_seats[train_num];
                    }
                    cout << "Thread " << thread_num << ": booking " << num_booked << " seats in " << train_num << std::endl;
                    available_seats[train_num] -= num_booked;
                    cout << "Thread " << thread_num << ": booked " << num_booked << " seats in " << train_num << std::endl;
                } else {
                    cout << "Thread " << thread_num << ": " << train_num << " has no available seats" << std::endl;
                }
                // print_mutex.unlock();
                break;
            case 3:
                if (available_seats[train_num] < 500) {
                    int num_cancelled = 500 - available_seats[train_num];
                    num_cancelled=rand() % num_cancelled + 1;
                    cout << "Thread " << thread_num << ": cancelling " << num_cancelled << " seats in " << train_num << std::endl;
                    available_seats[train_num] += num_cancelled;
                    cout << "Thread " << thread_num << ": cancelled " << num_cancelled << " seats in " << train_num << std::endl;
                } else {
                    cout << "Thread " << thread_num << ": no bookings to cancel in " << train_num << std::endl;
                }
                // print_mutex.unlock();
                break;
            default:
                cout << "Thread " << thread_num << ": Invalid query type" << std::endl;
                // print_mutex.unlock();
        }
        print_mutex.unlock();
        lock.lock();
        for (int i = 0; i < num_active_queries; i++) {
            if (active_queries[i].thread_num == thread_num) {
                active_queries[i] = active_queries[--num_active_queries];
                break;
            }
        }
        query_cond.notify_all();
        lock.unlock();
        auto end = std::chrono::steady_clock::now();
        auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(end - start);
        if (elapsed_seconds.count() >= MAX_TIME * 60) {
            break;
        }
    }
}

int main() {
    std::srand(std::time(nullptr));
    for (int i = 0; i < MAX_TRAINS; i++) {
        available_seats[i] = CAPACITY;
    }
    for (int i = 0; i < MAX_THREADS; i++) {
        threads[i] = std::thread(worker_thread, i);
        num_threads++;
    }
    for (int i = 0; i < num_threads; i++) {
        threads[i].join();
    }
    cout<<"The reservation chart at the end\n";
    cout<<"    Train number"<<" "<<"Available Seats\n";
    for(int i=0;i<MAX_TRAINS;i++){
        cout<<"     "<<i<<"             "<<available_seats[i]<<std::endl;
    }
    cout<<"Thanks for using our services!!!\n";
    return 0;
}
