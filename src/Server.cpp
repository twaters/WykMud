#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <thread>


#include "WykMud/Server.hpp"

namespace wyk
{
Server::Server(std::uint16_t listenPort, std::uint16_t maxConnections)
  : m_listen(false),
    m_listenSocketFd(-1),
    m_listenPort(listenPort),
    m_maxConnections(maxConnections),
    m_pServerInfo(nullptr),
    m_sockets(),
    m_sessions()
{
}

Server::~Server()
{
  close();
  cleanup();
}

bool Server::open()
{
  std::lock_guard<std::mutex> l(m_mutListenThread);
  bool ok = false;

  if (!m_listen)
  {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // Get our own server info
    int res = getaddrinfo(NULL, std::to_string(m_listenPort).c_str(), &hints, &m_pServerInfo);
    if (res < 0)
    {
      std::cerr << "getaddrinfo error: " << gai_strerror(res) << std::endl;
      return false;
    }

    // Create a socket on our server
    m_listenSocketFd = socket(m_pServerInfo->ai_family, m_pServerInfo->ai_socktype, m_pServerInfo->ai_protocol);
    if (m_listenSocketFd <= 0)
    {
      std::cerr << "socket error: " << strerror(errno) << std::endl;
      cleanup();
      return false;
    }

    // Make the listen socket non-blocking so accept() doesn't hang if nothing is there
    fcntl(m_listenSocketFd, F_SETFL, O_NONBLOCK);

    // Allow re-using a port in case we restart quickly
    int yes = 1;
    setsockopt(m_listenSocketFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    // Reserve the port
    res = bind(m_listenSocketFd, m_pServerInfo->ai_addr, m_pServerInfo->ai_addrlen);
    if (res < 0)
    {
      std::cerr << "bind error " << strerror(errno) << std::endl;
      cleanup();
      return false;
    }

    // Allow listening for connections
    res = ::listen(m_listenSocketFd, 20);
    if (res < 0)
    {
      std::cerr << "listen error " << strerror(errno) << std::endl;
      cleanup();
      return false;
    }

    addPoll(m_listenSocketFd);

    m_listen = true;
    ok = true;
  }
  else
  {
    std::cerr << "Server is already running" << std::endl;
  }

  return ok;
}

void Server::close()
{
  std::lock_guard<std::mutex> l(m_mutListenThread);

  m_listen = false;
}

void Server::cleanup()
{
  if (m_pServerInfo)
  {
    freeaddrinfo(m_pServerInfo);
    m_pServerInfo = nullptr;
  }

  for (const struct pollfd pfd : m_sockets)
  {
    ::close(pfd.fd);
  }
  m_sockets.clear();

  m_listenSocketFd = -1;
}


void Server::listen()
{
  std::map<int, bool> removeFds;
  bool newConnections;
  while (m_listen)
  {
    newConnections = false;
    // Block until there is data to be read, a new connection waiting, or
    // a signal occurred
    int num = poll(m_sockets.data(), m_sockets.size(), -1);
    for (std::uint32_t i(0); i < m_sockets.size(); i++)
    {
      if (m_sockets[i].revents != 0)
      {
        if (m_sockets[i].fd == m_listenSocketFd)
        {
          newConnections = true;
        }
        else if (!manageConnection(m_sockets[i]))
        {
          removeFds[m_sockets[i].fd] = true;
        }
      }
    }

    m_sockets.erase(std::remove_if(m_sockets.begin(), m_sockets.end(), [&](const struct pollfd& socket)
    {
      return removeFds.find(socket.fd) != removeFds.end();
    }), m_sockets.end());
    removeFds.clear();

    if (newConnections)
    {
      acceptConnections();
    }
  }
}

void Server::acceptConnections()
{
  int newSocket;
  do
  {
    newSocket = accept(m_listenSocketFd, NULL, NULL);
    if (newSocket > 0 && addPoll(newSocket))
    {
      m_sessions[newSocket] = std::make_unique<Session>(newSocket);
    }
    else if (newSocket > 0)
    {
      ::close(newSocket);
    }
  } while (newSocket > 0);
}

bool Server::manageConnection(struct pollfd& rFd)
{
  if (rFd.revents & (POLLRDHUP | POLLERR | POLLHUP))
  {
    std::cout << rFd.fd << " hung up" << std::endl;
    ::close(rFd.fd);
    m_sessions.erase(m_sessions.find(rFd.fd));
    return false;
  }
  else if (rFd.revents & POLLIN)
  {
    char buf[500];
    memset(buf, 0, 500);
    ssize_t readSize = recv(rFd.fd, (void*)buf, sizeof(buf), 0);
    if (readSize > 0)
    {
      m_sessions[rFd.fd]->processCommand(std::string(buf, readSize));
      return true;
    }
    else
    {
      ::close(rFd.fd);
      return false;
    }
  }
  return false;
}

bool Server::addPoll(int sockFd)
{
  struct pollfd fd;
  fd.fd = sockFd;
  fd.events = POLLIN | POLLHUP | POLLERR | POLLRDHUP;
  fd.revents = 0;

  if (m_sockets.size() < m_maxConnections)
  {
    m_sockets.push_back(fd);
    return true;
  }
  else
  {
    std::cerr << "too many connections" << std::endl;
    return false;
  }
}
} // namespace wyk
