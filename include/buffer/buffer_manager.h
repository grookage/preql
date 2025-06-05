#pragma once

#include <string>
#include <memory>
#include <cstdint>

namespace preql {
namespace buffer {

class BufferManager {
public:
    BufferManager();
    ~BufferManager();

    // Buffer initialization and cleanup
    bool initialize(size_t size_kb);
    void cleanup();

    // Page operations
    bool readPage(const std::string& db_name, uint32_t page_num);
    bool writePage(const std::string& db_name, uint32_t page_num);
    bool commit(const std::string& db_name, uint32_t page_num);
    bool commitAll(const std::string& db_name);

    // Frame operations
    void showFrame(uint32_t frame_num) const;
    void showFrames() const;

    // Buffer statistics
    size_t getBufferSize() const;
    size_t getFreeFrames() const;
    size_t getUsedFrames() const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace buffer
} // namespace preql 