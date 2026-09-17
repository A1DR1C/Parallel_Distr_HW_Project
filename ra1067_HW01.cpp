/*

HW01:
Author: Royce Arokia-Raj
Cite:

1. Run the code single threaded
2. benchmark the code multi threaded writing to *pcount every time (Slower)
3. benchmark the code multi threaded with a pool of 2 threads (4 optional)
4. Now you write partitioning. Divide not into 2 but into chunks

Put the table of results in the comment.


*/
#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <atomic>
#include <vector>
#include <algorithm>
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
    for (int i = 3; i <= n; i+=2) //O(n)
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
    for (int i = a; i <= b; i+=2) //O(n)
      if (isPrime(i)) //O(sqrt(n))
        (*pcount)++;
}


// do the counting in a register, do not write to memory
void countPrimesMultithreaded2(uint64_t a, uint64_t b, uint64_t* pcount) {
    uint64_t count = (a == 2 ? 1 : 0);
    a |= 1; // 10101010101010101010101010100001
  // THE ABOVE GUARANTEES THAT a IS odd 
    for (int i = a; i <= b; i+=2) //O(n)
      if (isPrime(i)) //O(sqrt(n))
        count++;
    *pcount = count;
}


atomic<uint64_t> nextChunk(0);

// each pool thread runs this: keeps grabbing chunks until none are left
void worker(uint64_t n, uint64_t chunkSize, uint64_t numChunks,
            vector<uint64_t>& counts) {
    uint64_t c;
    while ((c = nextChunk++) < numChunks) {
        uint64_t a = 2 + c * chunkSize;
        uint64_t b = min(a + chunkSize - 1, n);
        countPrimesMultithreaded2(a, b, &counts[c]);
    }
}


int main(int argc, char* argv[]) {
    uint64_t n = atol(argv[1]);
    uint64_t chunkSize = 1024*1024;
//     uint64_t current = 2;
// //    std::cout << countPrimes(n) << '\n';
//     uint64_t count1 = 0, count2 = 0;
//     thread t1(countPrimesMultithreaded2, 2, n/2, &count1);
//     thread t2(countPrimesMultithreaded2, n/2+1, n, &count2);
//     t1.join();
//     t2.join();
//     uint64_t count = count1 + count2;
//     cout << count << '\n';

    uint64_t numChunks = (n - 1) / chunkSize + 1;
    int numThreads = 2;

    vector<uint64_t> counts(numChunks, 0);
    vector<thread> threads;

    nextChunk = 0;
    for (int i = 0; i < numThreads; i++)
        threads.emplace_back(worker, n, chunkSize, numChunks, ref(counts));

    for (auto& t : threads)
        t.join();

    uint64_t count = 0;
    for (auto c : counts)
        count += c;

    cout << count << '\n';

}