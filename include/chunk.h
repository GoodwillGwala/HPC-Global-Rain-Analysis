#ifndef _CHUNK_H_
#define _CHUNK_H_

// Currently only 1D array chunk is supported.
struct Chunk
{
    size_t start = 1;
    size_t length = 0;

    Chunk() {}
    Chunk(size_t s, size_t l) : start(s), length(l) {}

    bool empty() const
    {
        return length == 0;
    }
};

#endif  // _CHUNK_H_
