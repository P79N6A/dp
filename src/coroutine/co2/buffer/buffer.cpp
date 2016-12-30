/*
 * buffer.cpp
 * Created on: 2016-12-13
 */

#include "co2/buffer/buffer.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <string>

#include "co2/co2.h"

#define LINE_FOUND          0
#define LINE_NOT_FOUND      1
#define LINE_ALMOST_FOUND   2

namespace co2 {
namespace buffer {

using namespace co2;

/* helper function to search a character in string,
 * because glibc doesn't provide one for us. 
 * */
static char *strnchr(const char *src, int len, char c);

Pool::Pool(int buff_size, int batch_size) :
    buff_size_(buff_size), batch_size_(batch_size),
    avail_count_(0), total_count_(0),
    block_(NULL), head_(NULL)
{

}

Pool::~Pool(void)
{
    Destroy();
}

int Pool::Alloc(void)
{
    int size = sizeof(buffer_block_ *) +
        (sizeof(buffer_) + buff_size_) * batch_size_;

    co2::Log(INFO, "alloc %d bytes\n", size);

    char *block = (char *)malloc(size);

    if (!block)
        return kMemoryErr;

    /* set head of block */
    ((struct buffer_block_ *)block)->next = block_;
    block_ = (struct buffer_block_ *)block;

    int i;
    struct buffer_ *buf;
    block += sizeof(struct buffer_block *);

    /* cut the block into pieces */
    for (i = batch_size_ - 1; i >= 0; i--) {
        buf = (struct buffer_ *)((char *)block +
            (i * (sizeof(struct buffer_) + buff_size_)));

        buf->next = head_;
        buf->buf = (char *)buf + sizeof(struct buffer_);
        head_ = buf;
    }

    avail_count_ += batch_size_;
    total_count_ += batch_size_;
    return kOK;
}

int Pool::Destroy(void)
{
    struct buffer_block_ *block = block_;
    while (block) {
        block_ = block->next;
        free(block);
        block = block_;
    }
    block_ = NULL;
    avail_count_ = 0;
    total_count_ = 0;
    return 0;
}

struct buffer_ *Pool::GetBuffer(void)
{
    if (!head_) {
        if (Alloc() != 0)
            return NULL;
    }
    struct buffer_ *buf = head_;
    head_ = buf->next;

    buf->beg = 0;
    buf->end = 0;
    buf->size = buff_size_;
    buf->next = NULL;
    buf->buf = (char *)buf + sizeof(struct buffer_);
    return buf;
}

void Pool::Release(buffer_ *buf)
{
    /* simple check if the buf is displaced. */
    // assert(buf->size == buff_size_);

    buf->next = head_;
    head_ = buf;
}

Buffer::Buffer(Pool *pool, int max_size) : pool_(pool),
    head_(NULL), tail_(NULL),
    size_(0), max_size_(max_size), ln_mode_(Buffer::kCR)
{

}

Buffer::~Buffer(void)
{
    struct buffer_ *buf;
    while (head_) {
        buf = head_->next;
        pool_->Release(head_);
        head_ = buf;
    }
}

char * strnchr(const char *src, int len, char c)
{
    int i = 0;
    for (; i < len; ++i) {
        if (src[i] == c)
            return (char *)(src + i);
    }
    return NULL;
}

/* read a line to buffer (end with \n or \r\n)
 * TODO: should we write some code to manage the
 * buffer piece? */
int Buffer::ReadLine(int fd, std::string &obuf)
{
    int bytes = 0;
    int stat = GetLinePos(bytes);

    assert(bytes <= size_);

    while (stat != LINE_FOUND) {
        if (bytes >= max_size_)
            return kMemoryErr;

        if (!head_ || tail_->end == tail_->size) {
            struct buffer_ *buf = pool_->GetBuffer();
            if (!buf) {
                return kMemoryErr;
            }
            if (!head_) {
                head_ = tail_ = buf;
            } else {
                tail_->next = buf;
                tail_ = buf;
            }
        }
        char *p = tail_->buf + tail_->end;
        while (true) {
            /* read slow */
            assert(tail_->size > tail_->end);

            char *c;
            int r = read(fd, p, tail_->size - tail_->end);
            if (r < 0) {
                return kReadErr;
            } else if (r == 0) {
                return errno == ETIME ? kTimeoutErr : kClosedErr;
            }

            size_ += r;
            tail_->end += r;

            c = strnchr(p, tail_->size - tail_->end + r,
                ln_mode_ == kLF ? '\r' : '\n');

            if (c == NULL) {
                bytes += r;
                p += r;

                if (ln_mode_ == kLFCR && tail_->buf[tail_->end - 1] == '\r') {
                    stat = LINE_ALMOST_FOUND;
                } else {
                    stat = LINE_NOT_FOUND;
                }
                if (tail_->end == tail_->size) {
                    struct buffer_ *buf = pool_->GetBuffer();
                    if (!buf) {
                        return kMemoryErr;
                    }
                    tail_->next = buf;
                    tail_ = buf;
                    break;
                }
            } else if (ln_mode_ != kLFCR) {
                bytes += c - p + 1;
                stat = LINE_FOUND;
                break;
            } else if (stat == LINE_ALMOST_FOUND) { /* LFCR */
                if (c[0] == '\n') {
                    bytes++;
                    stat = LINE_FOUND;
                    break;
                } else if (c[-1] == '\r') {
                    bytes += c - p + 1;
                    stat = LINE_FOUND;
                    break;
                }
                bytes += c - p + 1;
                stat = LINE_NOT_FOUND;
                /* one char pass n */
                p = c + 1;
            } else {
                if (c - p >= 1 && c[-1] == '\r') {
                    bytes += c - p + 1;
                    stat = LINE_FOUND;
                    break;
                }
                bytes += c - p + 1;
                stat = LINE_NOT_FOUND;
                p = c + 1;
            }
        }
    }
    obuf.resize(bytes);
    ReadBytes(fd, const_cast<char *>(obuf.c_str()), bytes);
    return 0;
}

/* get the line pos
 * return 0 for a successful search and the pos is how many bytes
 * we have to read.
 *
 * 1 to hint that not found.
 * 2 to hint that almost there(used by lfcr)
 * */
int Buffer::GetLinePos(int &pos)
{
    struct buffer_ *buf = head_;
    char *p;
    pos = 0;

    char c = ln_mode_ == kCR ? '\n' : 'r';

    if (!buf) {
        return LINE_NOT_FOUND;
    }

    p = buf->buf + buf->beg;
    for (;;) {
        p = strnchr(p, buf->end - buf->beg, c);
        if (p == NULL) { /* not found */
            if (!buf->next) {
                return LINE_NOT_FOUND;
            }

            assert(buf->end == buf->size);
            buf = buf->next;

            assert(buf->beg == 0);
            p = buf->buf; /* search for new buffer */
            continue;
        }

        /* found or almost found */
        if (ln_mode_ != Buffer::kLFCR) {
            /* one byte pass the char */
            pos += p - (buf->buf + buf->beg) + 1;
            return 0;
        } else {
            /* LFCR mode and almost found. */
            if (p != buf->buf + buf->end - 1) {
                if (p[1] == '\n') {
                    pos += p - (buf->buf + buf->beg) + 2;
                    assert(pos == size_);
                    return 0;
                } else {
                    /* search from this position. don't add pos. */
                    p += 1;
                }
            } else {
                /* just an LF, damn it! have to write more code.
                 * now that we are at the end of the buffer, check
                 * to see if the \r\n straddle two buffers. */
                pos += buf->end - buf->beg;
                buf = buf->next;
                if (!buf || !buf->end)
                    return LINE_ALMOST_FOUND;

                if (buf->buf[0] == '\n') { /* the first char is \n */
                    pos++; /* add\n to pos */
                    return 0;
                }
                p = buf->buf;
            }
        }
    }
    return pos;
}

int Buffer::ReadBytes(int fd, std::string &obuf, int bytes)
{
    obuf.resize(bytes);
    return ReadBytes(fd, const_cast<char *>(obuf.c_str()), bytes);
}

/* read bytes into buffer p, p must have enough
 * space to hold the data */
int Buffer::ReadBytes(int fd, char *p, int bytes)
{
    while (bytes) {
        struct buffer_ *buf = head_;
        if (!head_) {
            buf = pool_->GetBuffer();
            if (!buf) {
                return kMemoryErr;
            }
            head_ = tail_ = buf;
        }

        if (!size_) {
            /* if !size, beg and end should both be 0 */
            assert(buf->beg == 0 && buf->end == 0);

            /* read slow: we should reduce the read call as much as we can. */
            int r = read(fd, buf->buf, buf->size);
            if (r == -1) {
                /* client close or timeout */
                return kReadErr;
            } else if (r == 0) {
                return errno == ETIME ? kTimeoutErr : kClosedErr;
            }
            buf->end = r;
            size_ = r;
        }

        if (buf->end - buf->beg > bytes) {
            /* we are done here */
            memcpy(p, buf->buf + buf->beg, bytes);
            buf->beg += bytes;
            size_ -= bytes;
            bytes = 0;
        } else {
            memcpy(p, buf->buf + buf->beg, buf->end - buf->beg);
            p     += buf->end - buf->beg;
            bytes -= buf->end - buf->beg;
            size_ -= buf->end - buf->beg;

            /* make it a cyclic queue */
            if (buf->end == buf->size) {
                if (!buf->next) {
                    buf->beg = 0;
                    buf->end = 0;
                } else {
                    head_ = buf->next;
                    pool_->Release(buf);
                }
            } else {
                buf->beg = 0;
                buf->end = 0;
            }
        }
    }
    return kOK;
}

int Buffer::WriteBytes(const char *src, int bytes)
{
    while (bytes > 0) {
        if (!head_) {
            head_ = pool_->GetBuffer();
            if (!head_)
                return kMemoryErr;

            tail_ = head_;
        }

        int space = tail_->size - tail_->end;
        if (space < bytes) {
            struct buffer_ *buf = pool_->GetBuffer();
            if (!buf) {
                return kMemoryErr;
            }
            memcpy(tail_->buf + tail_->end, src, space);
            bytes -= space;
            size_ += space;

            tail_->end = tail_->size;
            tail_->next = buf;
            tail_ = buf;
        } else {
            memcpy(tail_->buf + tail_->end, src, bytes);
            size_ += bytes;
            tail_->end += bytes;
            break;
        }
    }
    return kOK;
}

int Buffer::Flush(int fd)
{
    struct buffer_ *buf = head_;
    int res;
    while (buf) {
        res = write(fd, buf->buf + buf->beg, buf->end - buf->beg);
        if (res < 0)
            return kWriteErr;

        size_ -= res;
        if (res == buf->end - buf->beg) {
            head_ = head_->next;
            pool_->Release(buf);
            buf = head_;
        } else {
            buf->beg += res;
        }
    }

    assert(size_ == 0);
    return 0;
}

}  // namespace buffer
}  // namespace co2


