// Iurii Kondrakov
// TBB_Main.cpp
// 2021.12.07

#include <iostream>
#include <chrono>
#include <tbb/tbb.h>
#include <tbb/parallel_reduce.h>

template<typename T, typename C>
class Body {
    T accumul_;
    const T* const in_;
    T* const out_;
    const T identity_;
    const C combine_;

public:
    Body(T* out, const T* in, T i, C c) :
        accumul_(i), identity_(i),
        in_(in), out_(out),
        combine_(c) {}

    Body(Body& src, tbb::split) :
        accumul_(src.identity_), identity_(src.identity_),
        in_(src.in_), out_(src.out_),
        combine_(src.combine_) {}

    template<typename Tag>
    void operator() (const tbb::blocked_range<T>& r, Tag) {
        T temp = accumul_;
        for (auto i = r.begin(); i < r.end(); i++) {
            if (Tag::is_final_scan())
                out_[i] = temp;
            temp = combine_(temp, in_[i]);
        }
        accumul_ = temp;
    }

    T get_accumul() {
        return accumul_;
    }

    void reverse_join(Body& src) {
        accumul_ = combine_(accumul_, src.accumul_);
    }

    void assign(Body& src) {
        accumul_ = src.accumul_;
    }
};


// report system time
void reportTime(const char* msg, std::chrono::steady_clock::duration span) {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(span);
    std::cout << msg << " - took - " <<
        ms.count() << " milliseconds" << std::endl;
}

int main(int argc, char** argv) {
    if (argc > 3) {
        std::cerr << argv[0] << ": invalid number of arguments\n";
        std::cerr << "Usage: " << argv[0] << "\n";
        std::cerr << "Usage: " << argv[0] << " power_of_2\n";
        std::cerr << "Usage: " << argv[0] << " power_of_2 grainsize\n";
        return 1;
    }

    unsigned grainsize{ 0 };

    if (argc == 3) {
        grainsize = (unsigned)atoi(argv[2]);
        std::cout << "TBB Prefix Scan - grainsize = "
            << grainsize << std::endl;
    } else {
        std::cout << "TBB Prefix Scan - auto partitioning" << std::endl;
    }

    // initial values for testing
    const int N = 9;
    const int in_[N]{ 3, 1, 7, 0, 1, 4, 5, 9, 2 };

    // command line arguments - none for testing, 1 for large arrays
    int n;
    if (argc == 1) {
        n = N;
    }
    else {
        n = 1 << std::atoi(argv[1]);
        if (n < N) n = N;
    }
    int* in = new int[n];
    int* out = new int[n];

    // initialize
    for (int i = 0; i < N; i++)
        in[i] = in_[i];
    for (int i = N; i < n; i++)
        in[i] = 1;
    auto add = [](int a, int b) { return a + b; };

    // Inclusive Prefix Scan
    std::chrono::steady_clock::time_point ts, te;
    ts = std::chrono::steady_clock::now();
    Body<int, decltype(add)> body(out, in, 0, add);
    if (argc == 3)
        tbb::parallel_scan(tbb::blocked_range<int>(0, n, grainsize), body);
    else
        tbb::parallel_scan(tbb::blocked_range<int>(0, n), body);
    te = std::chrono::steady_clock::now();
    for (int i = 0; i < N; i++)
        std::cout << out[i] << ' ';
    std::cout << out[n - 1] << std::endl;
    reportTime("Exclusive Scan", te - ts);

    delete[] in;
    delete[] out;
}
