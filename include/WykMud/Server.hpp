#include <condition_variable>
#include <cstdint>
#include <map>
#include <mutex>
#include <queue>
#include <thread>

#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "WykMud/Session.hpp"

namespace wyk
{
class Server
{
  public:
    /**
     * @brief Initialize a server, but do not yet open the listening socket
     * @param[in] port The port to monitor for connections
     * @param[in] maxConnections The maximum number of allowed connections
     */
    explicit Server(std::uint16_t port, std::uint16_t maxConnections = 500);

    /**
     * @brief calls close()
     */
    virtual ~Server();

    /**
     * @brief Open the primary listening socket, making it available for connections
     * @return True if the listen port was successfully bound and the socket open. False on a failure
     */
    virtual bool open();

    /**
     * @brief Signal the listening loop to exit
     */
    virtual void close();

    /**
     * @brief Closes all connections/sockets - including the listening socket
     */
    virtual void cleanup();

    /**
     * @brief Polls on all sockets/connections for connection updates or data to be read.
     *        This is a blocking call in a while loop, that will run until close() is called
     */
    virtual void listen();

  protected:
    /**
     * @brief Accept any pending connections waiting on the connection listen socket
     */
    virtual void acceptConnections();

    /**
     * @brief Reads data on a connection or handles closing the server-side connetion
     *        on a hang-up or other socket error
     * @param[in] rFd The polling object representing the connection
     * @return True if the connection is still active. False if the connection was closed
     */
    virtual bool manageConnection(struct pollfd& rFd);

    /**
     * @brief Attempts to add a new socket descriptor to the list of managed polling objects
     *        (i.e. addsa  new monitored connection). Verifies the maximum connections have not been
     *        exceeded. Sets the polling events as needed by listen() and manageConnection(). If the
     *        the number of connections was exceeded, it is the caller's responsibility to close the
     *        descriptor.
     * @param[in] sockFd The new socket descriptor to begin monitoring
     * @return True if the maximum connections were not exceeded. False if the maximum connections
     *         was exceeded and the socket closed
     */
    bool addPoll(int sockFd);

    bool m_listen;                //!< Signals when listen() should exit. Set by calling open() or close()
    std::mutex m_mutListenThread; //!< Protects access to m_listen and the listen() method

    int m_listenSocketFd;                 //!< File descriptor of the connection-monitoring socket
    std::uint16_t m_listenPort;           //!< The port to listen on
    std::uint16_t m_maxConnections;       //!< The maximum number of allowed connections
    struct addrinfo* m_pServerInfo;       //!< Populated addrinfo with our local server address/port populated
    std::vector<struct pollfd> m_sockets; //!< Polling structures representing active connections
    std::map<int, std::unique_ptr<Session>> m_sessions; //!< Logged-in sessions

    /// @cond
    Server(const Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(const Server&) = delete;
    Server& operator=(Server&&) = delete;
    /// @endcond
};
} // namespace wyk
