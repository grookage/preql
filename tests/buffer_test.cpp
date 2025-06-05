#include <gtest/gtest.h>
#include "buffer/buffer_manager.h"
#include <fstream>
#include <filesystem>

using namespace preql;

class BufferManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test file
        std::ofstream file("test_file.db", std::ios::binary);
        file.write("test data", 9);
        file.close();
        
        buffer = std::make_unique<buffer::BufferManager>();
    }

    void TearDown() override {
        buffer.reset();
        std::filesystem::remove("test_file.db");
    }

    std::unique_ptr<buffer::BufferManager> buffer;
};

TEST_F(BufferManagerTest, ReadPage) {
    auto page = buffer->readPage("test_file.db", 0);
    ASSERT_NE(page, nullptr);
    
    // Verify page content
    std::string content(reinterpret_cast<char*>(page->data), 9);
    EXPECT_EQ(content, "test data");
}

TEST_F(BufferManagerTest, WritePage) {
    // Create a new page
    auto page = buffer->readPage("test_file.db", 0);
    ASSERT_NE(page, nullptr);
    
    // Modify page content
    std::string new_data = "new data";
    std::memcpy(page->data, new_data.c_str(), new_data.size());
    
    // Write page back
    EXPECT_TRUE(buffer->writePage("test_file.db", 0, page));
    
    // Read page again to verify
    auto verify_page = buffer->readPage("test_file.db", 0);
    ASSERT_NE(verify_page, nullptr);
    std::string content(reinterpret_cast<char*>(verify_page->data), new_data.size());
    EXPECT_EQ(content, "new data");
}

TEST_F(BufferManagerTest, PageReplacement) {
    // Create multiple pages to force page replacement
    const int num_pages = 100;
    std::vector<std::string> test_data;
    
    // Write multiple pages
    for (int i = 0; i < num_pages; ++i) {
        auto page = buffer->readPage("test_file.db", i);
        ASSERT_NE(page, nullptr);
        
        std::string data = "page " + std::to_string(i);
        test_data.push_back(data);
        std::memcpy(page->data, data.c_str(), data.size());
        
        EXPECT_TRUE(buffer->writePage("test_file.db", i, page));
    }
    
    // Read pages back to verify
    for (int i = 0; i < num_pages; ++i) {
        auto page = buffer->readPage("test_file.db", i);
        ASSERT_NE(page, nullptr);
        
        std::string content(reinterpret_cast<char*>(page->data), test_data[i].size());
        EXPECT_EQ(content, test_data[i]);
    }
}

TEST_F(BufferManagerTest, InvalidPageAccess) {
    // Try to read non-existent page
    auto page = buffer->readPage("test_file.db", 1000);
    EXPECT_EQ(page, nullptr);
    
    // Try to write to non-existent page
    auto new_page = std::make_unique<buffer::Page>();
    EXPECT_FALSE(buffer->writePage("test_file.db", 1000, new_page.get()));
}

TEST_F(BufferManagerTest, ConcurrentAccess) {
    const int num_threads = 4;
    const int pages_per_thread = 10;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, pages_per_thread]() {
            for (int i = 0; i < pages_per_thread; ++i) {
                int page_num = t * pages_per_thread + i;
                auto page = buffer->readPage("test_file.db", page_num);
                ASSERT_NE(page, nullptr);
                
                std::string data = "thread " + std::to_string(t) + " page " + std::to_string(i);
                std::memcpy(page->data, data.c_str(), data.size());
                
                EXPECT_TRUE(buffer->writePage("test_file.db", page_num, page));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all pages
    for (int t = 0; t < num_threads; ++t) {
        for (int i = 0; i < pages_per_thread; ++i) {
            int page_num = t * pages_per_thread + i;
            auto page = buffer->readPage("test_file.db", page_num);
            ASSERT_NE(page, nullptr);
            
            std::string expected = "thread " + std::to_string(t) + " page " + std::to_string(i);
            std::string content(reinterpret_cast<char*>(page->data), expected.size());
            EXPECT_EQ(content, expected);
        }
    }
}

TEST_F(BufferManagerTest, BufferPoolSize) {
    const int num_pages = 1000;
    std::vector<std::unique_ptr<buffer::Page>> pages;
    
    // Allocate more pages than buffer pool size
    for (int i = 0; i < num_pages; ++i) {
        auto page = buffer->readPage("test_file.db", i);
        ASSERT_NE(page, nullptr);
        pages.push_back(std::unique_ptr<buffer::Page>(page));
    }
    
    // Verify buffer pool size hasn't exceeded limit
    EXPECT_LE(buffer->getBufferPoolSize(), buffer->getMaxBufferPoolSize());
}

TEST_F(BufferManagerTest, PageDirtyFlag) {
    auto page = buffer->readPage("test_file.db", 0);
    ASSERT_NE(page, nullptr);
    
    // Modify page content
    std::string new_data = "new data";
    std::memcpy(page->data, new_data.c_str(), new_data.size());
    
    // Mark page as dirty
    page->is_dirty = true;
    
    // Write page back
    EXPECT_TRUE(buffer->writePage("test_file.db", 0, page));
    
    // Verify page is no longer dirty
    EXPECT_FALSE(page->is_dirty);
} 