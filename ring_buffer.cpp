#include <iostream>
#include <stdexcept>
#include <cstdint>

template <class T, size_t N>
class RingBuffer {
    T objects[N];
    size_t read = 0;
    size_t write = 0;
    size_t queued = 0;

public:
    RingBuffer() {}

    T& push() {
        T& current = objects[write];
        write = (write + 1) % N;
        if (queued < N) {
            queued++;
        } else {
            read = (read + 1) % N; // Overwrite the oldest data
        }
        return current;
    }

    const T& pull() {
        if (queued == 0) {
            throw std::runtime_error("No data in the ring buffer");
        }
        T& current = objects[read];
        read = (read + 1) % N;
        queued--;
        return current;
    }

    bool has_data() const { return queued > 0; }
};

struct Frame {
    uint32_t index;
    uint8_t data[1024];
};

int main() {
    RingBuffer<Frame, 10> frames;

    // Push data
    for (size_t i = 0; i < 5; i++) {
        Frame& out = frames.push();
        out.index = i;
        out.data[0] = 'A' + i;
        out.data[1] = '\0';
    }

    // Pull and print data
    while (frames.has_data()) {
        const Frame& in = frames.pull();
        std::cout << "Frame " << in.index << ": " << in.data << std::endl;
    }

    // Push more data to demonstrate overwriting
    for (size_t i = 0; i < 15; i++) {
        Frame& out = frames.push();
        out.index = i;
        out.data[0] = 'A' + (i % 26);
        out.data[1] = '\0';
    }

    // Pull and print the overwritten buffer
    while (frames.has_data()) {
        const Frame& in = frames.pull();
        std::cout << "Frame " << in.index << ": " << in.data << std::endl;
    }

    return 0;
}
