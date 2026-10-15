#include "ome/concurrency/mpsc_queue.hpp"
#include "ome/concurrency/spsc_queue.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

using namespace ome;

TEST(SpscQueue, PushPopRoundTrip) {
  SpscQueue<int> q(8);
  EXPECT_TRUE(q.try_push(1));
  EXPECT_TRUE(q.try_push(2));
  int v = 0;
  EXPECT_TRUE(q.try_pop(v));
  EXPECT_EQ(v, 1);
  EXPECT_TRUE(q.try_pop(v));
  EXPECT_EQ(v, 2);
  EXPECT_FALSE(q.try_pop(v));
}

TEST(MpscQueue, MultiProducerSingleConsumer) {
  MpscQueue<int> q(1024);
  constexpr int kProducers = 4;
  constexpr int kPerProducer = 1000;
  std::vector<std::thread> producers;
  for (int p = 0; p < kProducers; ++p) {
    producers.emplace_back([&, p] {
      for (int i = 0; i < kPerProducer; ++i) {
        const int value = p * kPerProducer + i;
        while (!q.try_push(value)) {
          std::this_thread::yield();
        }
      }
    });
  }

  std::vector<int> seen;
  seen.reserve(static_cast<std::size_t>(kProducers * kPerProducer));
  while (seen.size() < static_cast<std::size_t>(kProducers * kPerProducer)) {
    int v = 0;
    if (q.try_pop(v)) {
      seen.push_back(v);
    } else {
      std::this_thread::yield();
    }
  }

  for (auto& t : producers) {
    t.join();
  }

  std::vector<char> present(static_cast<std::size_t>(kProducers * kPerProducer), 0);
  for (int v : seen) {
    ASSERT_GE(v, 0);
    ASSERT_LT(v, kProducers * kPerProducer);
    present[static_cast<std::size_t>(v)] = 1;
  }
  for (char c : present) {
    EXPECT_EQ(c, 1);
  }
}
