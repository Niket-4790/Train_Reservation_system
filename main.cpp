#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstdlib> // For std::rand and std::srand

using namespace std;
using namespace std::chrono;

// --- DEFINITIONS ---
#define MAX_TRAINS 100
#define CAPACITY 500
#define BOOK_MIN 5
#define BOOK_MAX 10
#define MAX_THREADS 20
#define MAX_TIME 1 // mins

// --- GLOBAL SHARED RESOURCES ---
// 1. Mutexes to protect available_seats for *each* train
std::mutex train_mutex[MAX_TRAINS];

// 2. Train data
int available_seats[MAX_TRAINS];

// 3. Thread management
std::thread threads[MAX_THREADS];
int num_threads = 0;
std::mutex print_mutex; // For clean console output

// Removed: query_mutex, query_cond, active_queries, num_active_queries
// The active query pool and its logic are replaced by train-specific locks.

// --- HELPER FUNCTIONS (Unchanged) ---
int get_random_train() {
    return std::rand() % MAX_TRAINS;
}

int get_random_bookings() {
    return std::rand() % (BOOK_MAX - BOOK_MIN + 1) + BOOK_MIN;
}

void print_query(int thread_num, int type, int train_num) {
    lock_guard<std::mutex> lock(print_mutex);
    cout << "Thread " << thread_num << ": ";
    if (type == 1) {
        cout << "Inquiring " << train_num << endl;
    } else if (type == 2) {
        cout << "Attempting Booking on " << train_num << endl;
    } else if (type == 3) {
        cout << "Attempting Cancellation on " << train_num << endl;
    }
}

// --- WORKER THREAD (Modified) ---
void worker_thread(int thread_num) {
    auto start = std::chrono::steady_clock::now();

    while (true) {
        // 1. Simulate user thinking/activity
        std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() % 500));

        // 2. Generate Query
        int train_num = get_random_train();
        int type = std::rand() % 3 + 1; // 1: Inquire, 2: Book, 3: Cancel

        // Print intent before locking
        print_query(thread_num, type, train_num);

        // 3. CORE SYNCHRONIZATION: Acquire Lock for the specific train
        // For read/write access to available_seats[train_num]
        std::lock_guard<std::mutex> train_lock(train_mutex[train_num]);

        // 4. Execute Query (Inside the critical section)
        lock_guard<std::mutex> print_lock(print_mutex); // Lock for printing

        switch (type) {
            case 1: { // Inquiry (Read)
                // Read operation is safe because the train mutex is held.
                cout << "Thread " << thread_num << ": Train " << train_num
                     << " has " << available_seats[train_num] << " seats available." << endl;
                break;
            }
            case 2: { // Booking (Write)
                int num_to_book = get_random_bookings();
                if (available_seats[train_num] >= num_to_book) {
                    available_seats[train_num] -= num_to_book; // Write operation is safe.
                    cout << "Thread " << thread_num << ": SUCCESSFULLY BOOKED " << num_to_book
                         << " seats in Train " << train_num << ". Remaining: "
                         << available_seats[train_num] << endl;
                } else {
                    cout << "Thread " << thread_num << ": FAILED to book " << num_to_book
                         << " in Train " << train_num << ". Only "
                         << available_seats[train_num] << " available." << endl;
                }
                break;
            }
            case 3: { // Cancellation (Write)
                int booked_seats = CAPACITY - available_seats[train_num];
                if (booked_seats > 0) {
                    // Cancel a random number of seats up to the number booked
                    int num_to_cancel = std::rand() % booked_seats + 1;

                    available_seats[train_num] += num_to_cancel; // Write operation is safe.
                    cout << "Thread " << thread_num << ": SUCCESSFULLY CANCELLED " << num_to_cancel
                         << " seats in Train " << train_num << ". Remaining: "
                         << available_seats[train_num] << endl;
                } else {
                    cout << "Thread " << thread_num << ": Train " << train_num
                         << " has no bookings to cancel." << endl;
                }
                break;
            }
        }
        // 5. Release Locks: train_lock and print_lock are automatically released here
        // due to lock_guard going out of scope.

        // 6. Check Time Limit
        auto end = std::chrono::steady_clock::now();
        auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(end - start);
        if (elapsed_seconds.count() >= MAX_TIME * 60) {
            break;
        }
    }
}

// --- MAIN FUNCTION (Mostly Unchanged) ---
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

    cout << "\n--- Final Reservation Chart ---\n";
    cout << "    Train number    Available Seats\n";
    for(int i = 0; i < MAX_TRAINS; i++){
        cout << "        " << i << "                " << available_seats[i] << endl;
    }
    cout << "Thanks for using our services!!!\n";

    return 0;
}
