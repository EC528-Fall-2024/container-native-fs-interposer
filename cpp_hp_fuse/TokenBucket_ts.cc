#include <gtest/gtest.h>
#include <thread>

#include "TockenBucket.hpp"

class TokenBucketTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TokenBucketTest, SmallReadSize) {
    const size_t smallSize = tokenTroughput/2;
    TokenBucket bucket(smallSize, initialtoken);
    
    EXPECT_TRUE(bucket.enough_tokens());
}

TEST_F(TokenBucketTest, LargeReadSize) {
    const size_t largeSize = tokenTroughput * 3;
    TokenBucket bucket(largeSize, initialtoken);
    
    EXPECT_FALSE(bucket.enough_tokens());
    bucket.add_tokens();
    EXPECT_FALSE(bucket.enough_tokens());
    bucket.add_tokens();
    EXPECT_TRUE(bucket.enough_tokens());
}

TEST_F(TokenBucketTest, ExactTokenThroughputSize) {
    TokenBucket bucket(tokenTroughput, initialtoken);

    EXPECT_TRUE(bucket.enough_tokens());
}

// TEST_F(TokenBucketTest, MultipleSmallReads) {
//     const size_t smallSize = tokenTroughput / 4;
//     TokenBucket bucket(smallSize * 5, initialtoken);

//     EXPECT_TRUE(bucket.enough_tokens());
    
//     for (int i = 0; i < 4; ++i) {
//         bucket.consume_tokens(1);
//         EXPECT_TRUE(bucket.enough_tokens());
//     }
    
//     bucket.consume_tokens(1);
//     EXPECT_FALSE(bucket.enough_tokens());
// }

TEST_F(TokenBucketTest, TokenReplenishment) {
    const size_t largeSize = tokenTroughput * 2;
    TokenBucket bucket(largeSize, initialtoken);

    EXPECT_FALSE(bucket.enough_tokens());
    bucket.add_tokens(bucket.get_needs_tokens() - bucket.get_token_count());
    EXPECT_TRUE(bucket.enough_tokens());
}

TEST_F(TokenBucketTest, SetupTimerTest) {
    EXPECT_TRUE(setup_timer());
}

TEST_F(TokenBucketTest, TimerReplenishmentTest) {
    const size_t testSize = tokenTroughput * 2;
    auto bucket = std::make_shared<TokenBucket>(testSize, 0); // Start with 0 tokens
    active_buckets.push_back(bucket);

    EXPECT_FALSE(bucket->enough_tokens());

    ASSERT_TRUE(setup_timer());

    // Wait for 3 seconds to allow the timer to trigger a few times
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Check if tokens have been added
    EXPECT_GT(bucket->get_token_count(), 0);

    EXPECT_TRUE(bucket->enough_tokens());
}

GTEST_API_ int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}