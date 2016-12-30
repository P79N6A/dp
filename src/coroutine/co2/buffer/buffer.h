/*
 * buffer.h
 * Created on: 2016-12-13
 */

#ifndef CO2_BUFFER_BUFFER_H_
#define CO2_BUFFER_BUFFER_H_

#include <string>

namespace co2 {
namespace buffer {

struct buffer_ {
    struct buffer_ *next;
    char *buf;
    int beg; /* cyclic queue */
    int end;
    int size;
};

struct buffer_block_ {
    struct buffer_block_ *next;
};

class Pool {
public:
    Pool(int buff_size = 8192, int batch_size = 100);
    ~Pool(void);

    int Alloc(void);
    int Destroy(void);

    struct buffer_ *GetBuffer(void);
    void Release(buffer_ *buf);
    void Dump(void);

    int GetBuffCount(void) { return total_count_; }
    int GetAvailBuffCount(void) { return avail_count_; }
    int GetBuffSize(void)  { return buff_size_;  }
    int GetBatchSize(void) { return batch_size_; }

private:
    buffer_block_ *block_;
    buffer_ *head_;
    int total_count_;
    int avail_count_;
    int buff_size_;
    int batch_size_;
};

/* chained buffer, used by TCP connection */
class Buffer {
public:
    enum LINE_MODE {
        kLF,
        kCR,
        kLFCR,
    };

    /* max default to be 1MB */
    Buffer(Pool *pool, int max_size = 1024 * 1024);
    ~Buffer();

    void SetLineMode(LINE_MODE lm) { ln_mode_ = lm; }

    /* Buffer API
     * return 0 if successful and data will be stored in obuf. */
    int ReadBytes(int fd, char *obuf, int bytes);
    int ReadBytes(int fd, std::string &obuf, int bytes);

    /* read line from buffer and store into obuf */
    int ReadLine(int fd, std::string &obuf);

    /* append bytes to buffer, pipelining. */
    int WriteBytes(const char *buf, int bytes);

    /* flush buffer's content to network */
    int Flush(int fd);

    /* Debug method, dump the internal state of a buffer object. */
    void Dump(void);

private:
    /* Find the line mark. */
    int GetLinePos(int &pos);

private:
    Pool *pool_;
    buffer_ *head_;
    buffer_ *tail_;
    int size_; /* how many bytes */
    int max_size_;
    LINE_MODE ln_mode_;
};

}  // namespace buffer
}  // namespace co2



#endif /* CO2_BUFFER_BUFFER_H_ */
