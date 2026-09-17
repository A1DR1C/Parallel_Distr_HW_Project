/*

HW01:
Author: Hariharan Sethumadhavan, Royce Arokia Raj
Cite: ChatGPT 

1. Run the code single threaded
2. benchmark the code multi threaded writing to *pcount every time (Slower)
3. benchmark the code multi threaded with a pool of 2 threads (4 optional)
4. Now you write partitioning. Divide not into 2 but into chunks

Put the table of results in the comment.

Method: Time each complete run. Compare one thread, two equal halves, and
two threads taking alternating 1,048,576-number chunks. Each thread adds
locally; the totals are combined after join. Alternating chunks spreads the
slower large-number checks across both threads.

Benchmark results for n = 100000000:
Version                 Time (ms)        Prime count       Speedup
Single thread:           50840.1 ms       5761455          1.00x
2 threads, two halves:   33341.4 ms       5761455          1.52484x
2 threads, chunks:       26924.9 ms       5761455          1.88822x
*/

#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
using namespace std;

// example n = 1000000001
// O(sqrt n)  omega(1)
inline bool isPrime(uint64_t n) {
    uint64_t lim = sqrt(n);
    for (uint64_t i = 3; i <= lim; i+=2) {
      if (n % i == 0) // if n is evenly divisible by i
        return false;
    }
    return true;
}
// 3, 5, 7, 9, 11, 13, 15, 17, ... 1001, 1'000'000'000
uint64_t countPrimes(uint64_t n) { //O(n sqrt(n))
    uint64_t count = 1;
    for (uint64_t i = 3; i <= n; i+=2) //O(n)
      if (isPrime(i)) //O(sqrt(n))
        count++;
    return count;
}

// n = 10^9
// 2-1K, 1K+1-2K, 2K+1-3K, 

void countPrimesMultithreaded(uint64_t a, uint64_t b, uint64_t* pcount) {
//    uint64_t count = 1;
    *pcount = (a == 2 ? 1 : 0);
    a |= 1; // 10101010101010101010101010100001
  // THE ABOVE GUARANTEES THAT a IS odd 
//    if (a % 2 == 0)
//      a++;
    for (uint64_t i = a; i <= b; i+=2) //O(n)
      if (isPrime(i)) //O(sqrt(n))
        (*pcount)++;
}

// do the counting in a register, do not write to memory
void countPrimesMultithreaded2(uint64_t a, uint64_t b, uint64_t* pcount) {
    uint64_t count = (a == 2 ? 1 : 0);
    a |= 1; // 10101010101010101010101010100001
  // THE ABOVE GUARANTEES THAT a IS odd 
    for (uint64_t i = a; i <= b; i+=2) //O(n)
      if (isPrime(i)) //O(sqrt(n))
        count++;
    *pcount = count;
}



// My work: chunk assignment and benchmarks below.
// Worker 0 takes chunks 0, 2, ...; worker 1 takes chunks 1, 3, ... .
void countPrimeChunks(uint64_t n, uint64_t chunkSize,
                      uint64_t firstChunk, uint64_t chunkStride,
                      uint64_t* pcount) {
    uint64_t count = 0;
    uint64_t chunkStart = 2 + firstChunk * chunkSize;
    uint64_t chunkJump = chunkStride * chunkSize;

    while (chunkStart <= n) {
        uint64_t chunkEnd = chunkStart + chunkSize - 1;

        if (chunkEnd < chunkStart || chunkEnd > n)
            chunkEnd = n;

        uint64_t chunkCount = 0;
        countPrimesMultithreaded2(chunkStart, chunkEnd, &chunkCount);
        count += chunkCount;

        if (chunkJump > n - chunkStart)
            break;

        chunkStart += chunkJump;
    }

    *pcount = count;
}

uint64_t countPrimesParallel(uint64_t n, uint64_t chunkSize,
                             unsigned threadCount) {
    uint64_t counts[4] = {0, 0, 0, 0};
    thread workers[4];

    for (unsigned id = 0; id < threadCount; id++)
        workers[id] = thread(countPrimeChunks, n, chunkSize,
                             id, threadCount, &counts[id]);

    for (unsigned id = 0; id < threadCount; id++)
        workers[id].join();

    uint64_t total = 0;

    for (unsigned id = 0; id < threadCount; id++)
        total += counts[id];

    return total;
}

uint64_t countPrimesTwoHalves(uint64_t n) {
    uint64_t counts[2] = {0, 0};
    uint64_t middle = 2 + (n - 2) / 2;
    thread first(countPrimesMultithreaded2, 2, middle, &counts[0]);
    thread second(countPrimesMultithreaded2, middle + 1, n, &counts[1]);

    first.join();
    second.join();

    return counts[0] + counts[1];
}

double benchmarkSingle(uint64_t n, uint64_t* result) {
    auto startTime = chrono::steady_clock::now();
    *result = countPrimes(n);
    auto endTime = chrono::steady_clock::now();

    return chrono::duration<double, milli>(endTime - startTime).count();
}

double benchmarkParallel(uint64_t n, uint64_t chunkSize,
                         unsigned threadCount, uint64_t* result) {
    auto startTime = chrono::steady_clock::now();
    *result = countPrimesParallel(n, chunkSize, threadCount);
    auto endTime = chrono::steady_clock::now();

    return chrono::duration<double, milli>(endTime - startTime).count();
}

double benchmarkTwoHalves(uint64_t n, uint64_t* result) {
    auto startTime = chrono::steady_clock::now();
    *result = countPrimesTwoHalves(n);
    auto endTime = chrono::steady_clock::now();

    return chrono::duration<double, milli>(endTime - startTime).count();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <n>\n";
        return 1;
    }

    uint64_t n = atol(argv[1]);

    if (n < 2) {
        cout << 0 << '\n';
        return 0;
    }

    uint64_t chunkSize = 1024*1024;
    uint64_t singleCount = 0;
    uint64_t halfCount = 0;
    uint64_t count2 = 0;

    double singleTime = benchmarkSingle(n, &singleCount);
    double halfTime = benchmarkTwoHalves(n, &halfCount);
    double time2 = benchmarkParallel(n, chunkSize, 2, &count2);

    cout << "Single thread: " << singleTime << " ms, count = "
         << singleCount << ", speedup = 1.00x\n";
    cout << "2 threads, two halves: " << halfTime << " ms, count = "
         << halfCount << ", speedup = " << singleTime / halfTime << "x\n";
    cout << "2 threads, chunks: " << time2 << " ms, count = "
         << count2 << ", speedup = " << singleTime / time2 << "x\n";


}
