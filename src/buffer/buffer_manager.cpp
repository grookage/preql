#include "buffer/buffer_manager.h"
#include <algorithm>
#include <stdexcept>
#include <cstring>

namespace preql {
namespace buffer {

class BufferManager::Impl {
public:
    Impl() : buffer_size_(0), num_frames_(0) {}
    
    bool initialize(size_t size_kb) {
        if (buffer_size_ > 0) {
            return false;  // Already initialized
        }
        
        buffer_size_ = size_kb * KB;
        num_frames_ = buffer_size_ / PAGESIZE;
        
        // Initialize buffer pool
        buffer_pool_.resize(num_frames_);
        for (auto& frame : buffer_pool_) {
            frame.page_num = EMPTY;
            frame.is_dirty = false;
            frame.pin_count = 0;
            frame.last_used = 0;
            frame.data.resize(PAGESIZE);
        }
        
        return true;
    }
    
    void cleanup() {
        // Write all dirty pages
        for (size_t i = 0; i < num_frames_; ++i) {
            if (buffer_pool_[i].is_dirty && buffer_pool_[i].page_num != EMPTY) {
                writePage(buffer_pool_[i].db_name, buffer_pool_[i].page_num);
            }
        }
        
        buffer_pool_.clear();
        buffer_size_ = 0;
        num_frames_ = 0;
    }
    
    bool readPage(const std::string& db_name, uint32_t page_num) {
        if (buffer_size_ == 0) {
            return false;
        }
        
        // Check if page is already in buffer
        int frame_num = findPage(db_name, page_num);
        if (frame_num != -1) {
            buffer_pool_[frame_num].last_used = getCurrentTime();
            buffer_pool_[frame_num].pin_count++;
            return true;
        }
        
        // Find a free frame or victim frame
        frame_num = findFreeFrame();
        if (frame_num == -1) {
            frame_num = findVictimFrame();
            if (frame_num == -1) {
                return false;
            }
            
            // Write dirty page if necessary
            if (buffer_pool_[frame_num].is_dirty) {
                writePage(buffer_pool_[frame_num].db_name, 
                         buffer_pool_[frame_num].page_num);
            }
        }
        
        // Read page from disk
        std::string db_path = DBPATH + db_name;
        std::ifstream db_file(db_path, std::ios::binary);
        if (!db_file) {
            return false;
        }
        
        db_file.seekg(page_num * PAGESIZE);
        db_file.read(reinterpret_cast<char*>(buffer_pool_[frame_num].data.data()), 
                    PAGESIZE);
        
        // Update frame metadata
        buffer_pool_[frame_num].page_num = page_num;
        buffer_pool_[frame_num].db_name = db_name;
        buffer_pool_[frame_num].is_dirty = false;
        buffer_pool_[frame_num].pin_count = 1;
        buffer_pool_[frame_num].last_used = getCurrentTime();
        
        return true;
    }
    
    bool writePage(const std::string& db_name, uint32_t page_num) {
        if (buffer_size_ == 0) {
            return false;
        }
        
        int frame_num = findPage(db_name, page_num);
        if (frame_num == -1) {
            return false;
        }
        
        std::string db_path = DBPATH + db_name;
        std::ofstream db_file(db_path, std::ios::binary | std::ios::in);
        if (!db_file) {
            return false;
        }
        
        db_file.seekp(page_num * PAGESIZE);
        db_file.write(reinterpret_cast<const char*>(buffer_pool_[frame_num].data.data()), 
                     PAGESIZE);
        
        buffer_pool_[frame_num].is_dirty = false;
        return true;
    }
    
    bool commit(const std::string& db_name, uint32_t page_num) {
        return writePage(db_name, page_num);
    }
    
    bool commitAll(const std::string& db_name) {
        bool success = true;
        for (size_t i = 0; i < num_frames_; ++i) {
            if (buffer_pool_[i].db_name == db_name && buffer_pool_[i].is_dirty) {
                if (!writePage(db_name, buffer_pool_[i].page_num)) {
                    success = false;
                }
            }
        }
        return success;
    }
    
    void showFrame(uint32_t frame_num) const {
        if (frame_num >= num_frames_) {
            return;
        }
        
        const auto& frame = buffer_pool_[frame_num];
        std::cout << "Frame " << frame_num << ":\n"
                  << "  Page: " << frame.page_num << "\n"
                  << "  DB: " << frame.db_name << "\n"
                  << "  Dirty: " << (frame.is_dirty ? "Yes" : "No") << "\n"
                  << "  Pinned: " << frame.pin_count << "\n"
                  << "  Last Used: " << frame.last_used << "\n";
    }
    
    void showFrames() const {
        for (size_t i = 0; i < num_frames_; ++i) {
            showFrame(i);
        }
    }
    
    size_t getBufferSize() const {
        return buffer_size_;
    }
    
    size_t getFreeFrames() const {
        return std::count_if(buffer_pool_.begin(), buffer_pool_.end(),
                           [](const Frame& f) { return f.page_num == EMPTY; });
    }
    
    size_t getUsedFrames() const {
        return num_frames_ - getFreeFrames();
    }

private:
    struct Frame {
        uint32_t page_num;
        std::string db_name;
        bool is_dirty;
        int pin_count;
        uint64_t last_used;
        std::vector<char> data;
    };
    
    std::vector<Frame> buffer_pool_;
    size_t buffer_size_;
    size_t num_frames_;
    
    int findPage(const std::string& db_name, uint32_t page_num) const {
        for (size_t i = 0; i < num_frames_; ++i) {
            if (buffer_pool_[i].page_num == page_num && 
                buffer_pool_[i].db_name == db_name) {
                return i;
            }
        }
        return -1;
    }
    
    int findFreeFrame() const {
        for (size_t i = 0; i < num_frames_; ++i) {
            if (buffer_pool_[i].page_num == EMPTY) {
                return i;
            }
        }
        return -1;
    }
    
    int findVictimFrame() const {
        int victim = -1;
        uint64_t oldest_time = std::numeric_limits<uint64_t>::max();
        
        for (size_t i = 0; i < num_frames_; ++i) {
            if (buffer_pool_[i].pin_count == 0 && 
                buffer_pool_[i].last_used < oldest_time) {
                victim = i;
                oldest_time = buffer_pool_[i].last_used;
            }
        }
        
        return victim;
    }
    
    uint64_t getCurrentTime() const {
        return std::chrono::system_clock::now().time_since_epoch().count();
    }
};

// BufferManager class implementation
BufferManager::BufferManager() : pimpl_(std::make_unique<Impl>()) {}
BufferManager::~BufferManager() = default;

bool BufferManager::initialize(size_t size_kb) {
    return pimpl_->initialize(size_kb);
}

void BufferManager::cleanup() {
    pimpl_->cleanup();
}

bool BufferManager::readPage(const std::string& db_name, uint32_t page_num) {
    return pimpl_->readPage(db_name, page_num);
}

bool BufferManager::writePage(const std::string& db_name, uint32_t page_num) {
    return pimpl_->writePage(db_name, page_num);
}

bool BufferManager::commit(const std::string& db_name, uint32_t page_num) {
    return pimpl_->commit(db_name, page_num);
}

bool BufferManager::commitAll(const std::string& db_name) {
    return pimpl_->commitAll(db_name);
}

void BufferManager::showFrame(uint32_t frame_num) const {
    pimpl_->showFrame(frame_num);
}

void BufferManager::showFrames() const {
    pimpl_->showFrames();
}

size_t BufferManager::getBufferSize() const {
    return pimpl_->getBufferSize();
}

size_t BufferManager::getFreeFrames() const {
    return pimpl_->getFreeFrames();
}

size_t BufferManager::getUsedFrames() const {
    return pimpl_->getUsedFrames();
}

} // namespace buffer
} // namespace preql 