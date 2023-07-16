#include "benchmark/benchmark.h"

#include <atomic>
#include <mutex>
#include <thread>

template<typename T>
concept Lockable = requires(T m) {
	m.lock();
	m.unlock();
};

template<Lockable Lock>
static void BM_lock(benchmark::State& state) {
	static int counter = 0;
	static Lock lock;
	for (auto _ : state) {
		std::lock_guard guard(lock);
		++counter;
	}
}

class BadGreedySpinLock {
	std::atomic_flag flag_;

public:
	void lock() { while (flag_.test_and_set()); }
	void unlock() { flag_.clear(); }
};

class BadYieldingSpinLock {
	std::atomic_flag flag_;

public:
	void lock() { while (flag_.test_and_set()) std::this_thread::yield(); }
	void unlock() { flag_.clear(); }
};

static inline void cpu_pause() {
#if _MSC_VER
	_mm_pause();
#else
	__builtin_ia32_pause();
#endif
}

class SpinLock {
	std::atomic_flag flag_;

public:
	void lock() {
		for (;;) {
			if (!flag_.test_and_set(std::memory_order_acquire))
				return;
			while (flag_.test(std::memory_order_relaxed)) cpu_pause();
		}
	}
	void unlock() { flag_.clear(std::memory_order_release); }
};

class YieldingSpinLock {
	std::atomic_flag flag_;

public:
	void lock() {
		for (;;) {
			if (!flag_.test_and_set(std::memory_order_acquire))
				return;
			for (int spin_count = 0; flag_.test(std::memory_order_relaxed); ++spin_count) {
				if (spin_count < 16)
					cpu_pause();
				else {
					spin_count = 0;
					std::this_thread::yield();
				}
			}
		}
	}
	void unlock() { flag_.clear(std::memory_order_release); }
};

BENCHMARK(BM_lock<std::mutex>)->DenseThreadRange(1, 2 * std::thread::hardware_concurrency());
BENCHMARK(BM_lock<BadGreedySpinLock>)->DenseThreadRange(1, 2 * std::thread::hardware_concurrency());
BENCHMARK(BM_lock<BadYieldingSpinLock>)->DenseThreadRange(1, 2 * std::thread::hardware_concurrency());
BENCHMARK(BM_lock<SpinLock>)->DenseThreadRange(1, 2 * std::thread::hardware_concurrency());
BENCHMARK(BM_lock<YieldingSpinLock>)->DenseThreadRange(1, 2 * std::thread::hardware_concurrency());

BENCHMARK_MAIN();