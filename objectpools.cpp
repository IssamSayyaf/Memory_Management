#include <iostream>
#include <stdexcept>

template <class T, size_t N>
class ObjectPool {
    T objects[N];
    size_t available[N];
    size_t top = 0;

public:
    ObjectPool() {
        for (size_t i = 0; i < N; i++) {
            available[i] = i;
        }
    }

    T& get() {
        if (top < N) {
            return objects[available[top++]];
        }
        throw std::runtime_error("All objects are in use");
    }

    void free(const T& obj) {
        const T* ptr = &obj;
        size_t idx = (ptr - objects) / sizeof(T);
        if (idx < N && top > 0) {
            available[--top] = idx;
        } else {
            throw std::runtime_error("Invalid object");
        }
    }

    size_t requested() const { return top; }
};

struct Point {
    int x, y;
};

int main() {
    ObjectPool<Point, 10> points;

    Point& a = points.get();
    a.x = 10;
    a.y = 20;

    std::cout << "Point a: (" << a.x << ", " << a.y << ")" << std::endl;

    points.free(a);

    try {
        Point local;
        points.free(local);
    } catch (const std::runtime_error& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
