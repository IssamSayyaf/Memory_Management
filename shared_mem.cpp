#include <iostream>      // For standard I/O operations
#include <cstring>       // For std::fill_n
#include <sys/mman.h>    // For mmap, munmap, shm_open, shm_unlink
#include <fcntl.h>       // For file access flags (O_RDWR, O_CREAT)
#include <unistd.h>      // For close, fork
#include <stdexcept>     // For std::runtime_error
#include <thread>        // For std::this_thread::sleep_for
#include <chrono>        // For std::chrono::milliseconds

// Path to the shared memory object
const char* kSharedMemPath = "/sample_point"; // Unique name for the shared memory object
const size_t kPayloadSize = 16;               // Size of the raw data in bytes

// Structure to store the data in shared memory
struct Payload {
    uint32_t index;               // Index to identify the data frame
    uint8_t raw[kPayloadSize];    // Array to hold the payload data
};

// Template class for managing shared memory
template<class T>
class SharedMem {
    int fd;          // File descriptor for the shared memory object
    T* ptr;          // Pointer to the mapped shared memory region
    const char* name; // Name of the shared memory object (set if the object owner)

public:
    /**
     * Constructor
     * @param name - Name of the shared memory object.
     * @param owner - Boolean flag to indicate if this instance owns the shared memory.
     */
    SharedMem(const char* name, bool owner = false) {
        // Open or create the shared memory object
        fd = shm_open(name, O_RDWR | O_CREAT, 0600); // Read-write access, create if not exist, permission 0600
        if (fd == -1) {
            throw std::runtime_error("Failed to open shared memory");
        }

        // Set the size of the shared memory object
        if (ftruncate(fd, sizeof(T)) < 0) { // Truncate to the size of the template type `T`
            close(fd);                     // Close the file descriptor on error
            throw std::runtime_error("Failed to set shared memory size");
        }

        // Map the shared memory into the process's address space
        ptr = (T*)mmap(nullptr, sizeof(T), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (ptr == MAP_FAILED) {
            close(fd);                     // Close the file descriptor on error
            throw std::runtime_error("Failed to map shared memory");
        }

        // Store the name if the instance is the owner
        this->name = owner ? name : nullptr;
        std::cout << "Opened shared memory: " << name << std::endl;
    }

    /**
     * Destructor
     * Unmaps the shared memory and closes the file descriptor.
     * If the instance is the owner, also removes the shared memory object.
     */
    ~SharedMem() {
        munmap(ptr, sizeof(T));   // Unmap the shared memory region
        close(fd);                // Close the file descriptor
        if (name) {               // If owner, unlink (delete) the shared memory object
            std::cout << "Removing shared memory: " << name << std::endl;
            shm_unlink(name);     // Delete the shared memory object
        }
    }

    /**
     * @return Reference to the shared memory object.
     */
    T& get() const {
        return *ptr; // Return the dereferenced pointer to the shared object
    }
};

// Function for the producer to write data into shared memory
void producer() {
    SharedMem<Payload> writer(kSharedMemPath); // Create shared memory object (not the owner)
    Payload& pw = writer.get();               // Get reference to the shared object

    // Produce 5 frames of data
    for (int i = 0; i < 5; ++i) {
        pw.index = i;                         // Set the index of the data frame
        std::fill_n(pw.raw, kPayloadSize - 1, 'a' + i); // Fill raw data with 'a', 'b', ...
        pw.raw[kPayloadSize - 1] = '\0';      // Null-terminate the string
        std::cout << "Produced frame " << pw.index << ": " << pw.raw << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(150)); // Simulate production delay
    }
}

// Function for the consumer to read data from shared memory
void consumer() {
    SharedMem<Payload> reader(kSharedMemPath, true); // Create shared memory object (owner)
    Payload& pr = reader.get();                      // Get reference to the shared object

    // Consume 10 frames of data (may read duplicate frames due to timing mismatch)
    for (int i = 0; i < 10; ++i) {
        std::cout << "Consumed frame " << pr.index << ": " << pr.raw << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate consumption delay
    }
}

// Main function to fork the producer and consumer processes
int main() {
    if (fork() == 0) { // If in child process
        producer();    // Run producer
    } else {           // If in parent process
        consumer();    // Run consumer
    }
    return 0;
}
// +-----------------------+      +-----------------------+
// |   Producer Process    |      |   Consumer Process    |
// |-----------------------|      |-----------------------|
// |   shm_open()          |      |   shm_open()          |
// |   ftruncate()         |      |   mmap()              |
// |   mmap()              |      |                       |
// |   Write to SharedMem  |----->|   Read from SharedMem |
// +-----------------------+      +-----------------------+

//                    Shared Memory (Physical Memory)
//            +-----------------------------------------+
//            |   Payload:                             |
//            |   Index: 2                             |
//            |   Raw: "ccccc"                         |
//            +-----------------------------------------+
