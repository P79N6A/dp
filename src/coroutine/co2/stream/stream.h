/*
 * stream.h
 * Created on: 2016-11-21
 */

#ifndef CO2_STREAM_STREAM_H_
#define CO2_STREAM_STREAM_H_

#include <string.h>
#include <errno.h>

#include <string>
#include <functional>

#include "co2/buffer/buffer.h"

namespace co2 {
namespace stream {

using co2::buffer::Pool;
using co2::buffer::Buffer;

class Stream {
public:
    Stream(Pool *pool, int fd = -1, uint32_t max_buf_size = 1024 * 1024);
    ~Stream();

    /* @brief: Init Stream, allocate read/write buffer,
     * and make them nonblock. */
    int Init();

    /* @brief: Non-Blocking Connect to server
     * @param timeout: timeout in milliseconds,
     */
    int Connect(const std::string &addr, int port, int timeout = -1);

    /* @brief: Non-Blocking Connect to unix domain socket
     * @param timeout: timeout in milliseconds */
    int ConnectUnix(const std::string &path, int timeout = -1);

    /* @brief: read until delim is found, and call read_cb
     * @return $ret:
     *    0 for OK,
     *    kReadErr if read failed
     *    kTimeout if timeout
     *    kMemoryErr if not enough memory
     */
    int ReadBytes(int bytes, std::string &buf, int timeout = -1);

    /* @return $ret:
     *    0 for OK,
     *    kReadErr if read failed
     *    kTimeout if timeout
     *    kMemoryErr if not enough memory
     */
    int ReadLine(std::string &buf, int timeout = -1);

    /* @brief: write bytes to the network,
     * @return:
     *    0 if OK,
     *    kWriteErr if write error,
     *    kTimeout if timeout  */
    int WriteBytes(const char * buf, int bytes, int timeout = -1);
    int WriteBytes(const std::string &buf, int timeout = -1);

    /* @brief: append bytes to the write buffer. use this function
     *     to implement pipeline.
     * @return:
     *     0 if OK,
     *     kMemoryErr if not enough memory
     */
    int AppendBytes(const char *buf, int bytes);

    /* @brief: Flush all the data of the write buffer to the network
     * @return:
     *     0 if all the bytes are flushed,
     *     kWriteErr if something error when writing.
     */
    int Flush(int timeout = -1);

    /* @brief: close the underlying socket.
     * @return:
     *    0 if OK, non-zero otherwise
     */
    int Close(void);

    /* host and port util functions */
    const std::string& GetLocalHost(void);
    const std::string& GetRemoteHost(void);
    int GetLocalPort();
    int GetRemotePort();

    void SetLocalHost(const std::string &host) { local_host_ = host; }
    void SetRemoteHost(const std::string &host) { remote_host_ = host; }
    void SetLocalPort(int port) { local_port_ = port; }
    void SetRemotePort(int port) { remote_port_ = port; }

    int SetReadTimeout(int timeout);
    int SetWriteTimeout(int timeout);

private:
    /* delete constructors we don't need */
    Stream(Stream &&stream) = delete;
    Stream(const Stream &stream) = delete;
    Stream &operator=(const Stream &stream) = delete;
    Stream &operator=(Stream &&stream) = delete;

private:
    Pool *pool_;
    Buffer rbuf_;
    Buffer wbuf_;
    int fd_;

    std::string local_host_;
    std::string remote_host_;
    int local_port_;
    int remote_port_;

    int read_timeout_;
    int write_timeout_;
};

}  // namespace stream
}  // namespace co2


#endif /* CO2_STREAM_STREAM_H_ */
