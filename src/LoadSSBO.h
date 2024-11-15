#pragma once
#include <cstdint>
#include <glad/glad.h>

struct SSBO {
public:
    uint32_t GetHandle() const {
        return handle;
    }

    // Pre-allocate buffer storage
    void PreAllocate(size_t size) {
        if (handle != 0) {
            // If handle already exists, resize the buffer
            Update(size, nullptr);
            return;
        }
        glCreateBuffers(1, &handle);
        glNamedBufferStorage(handle, (GLsizeiptr)size, nullptr, GL_DYNAMIC_STORAGE_BIT);
        bufferSize = size;
    }

    // Update buffer data with new data
    void Update(size_t size, void* data) {
        if (size == 0) return;

        if (handle == 0) {
            PreAllocate(size);
        }

        // If the buffer size is less than the new data size, reallocate
        if (bufferSize < size) {
            glDeleteBuffers(1, &handle);
            PreAllocate(size);  // Reallocate the buffer with new size
        }

        // Update buffer data (you could use glMapBufferRange for performance gains with large data)
        if (data != nullptr) {
          glNamedBufferSubData(handle, 0, (GLsizeiptr)size, data);
        } 
    }

    // Clean up the buffer when done
    void CleanUp() {
        if (handle != 0) {
            glDeleteBuffers(1, &handle);
            handle = 0;
            bufferSize = 0;
        }
    }

    // Optionally, map the buffer into memory for direct access
    void* MapBuffer() {
        if (handle == 0) return nullptr;
        return glMapNamedBuffer(handle, GL_READ_WRITE);
    }

    // Optionally, unmap the buffer after mapping
    void UnmapBuffer() {
        if (handle != 0) {
            glUnmapNamedBuffer(handle);
        }
    }

private:
    uint32_t handle = 0;
    size_t bufferSize = 0;
};
