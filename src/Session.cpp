#include <sys/socket.h>

#include <iostream>

#include "WykMud/Session.hpp"

namespace
{
void sendFd(int fd, const std::string& message)
{
  ssize_t sent = send(fd, message.c_str(), message.size(), 0);
  if (sent != message.size())
  {
    std::cerr << "error sending message: " << message << std::endl;
  }
}
} // namespace

namespace wyk
{
Session::Session(int fd)
  : m_fd(fd),
    m_state(REQ_USERNAME)
{
  sendFd(fd, "Username (or NEW): ");
}

Session::~Session()
{
  std::cout << "Closing session " << fd() << std::endl;
}

bool Session::processCommand(const std::string& cmd)
{
  bool ok{false};

  if (ok)
  {
    m_lastCmdTime = std::chrono::system_clock::now();
  }

  switch (m_state)
  {
    case REQ_USERNAME:
      m_name = cmd;
      m_name = m_name.erase(m_name.size() - 2);
      std::cout << "name: " << m_name << std::endl;
      if (m_name == "NEW")
      {
        m_state = CREATE_USER;
        sendFd(m_fd, "What name would you like? ");
      }
      else
      {
        m_state = AUTHENTICATE_USER;
        sendFd(m_fd, "Password: ");
      }
      break;
    case AUTHENTICATE_USER:
      m_state = AUTHENTICATED;
      sendFd(m_fd, "Welcome!!");
      break;
    case CREATE_USER:
      m_name = cmd;
      m_state = CREATE_PASSWORD;
      sendFd(m_fd, "Choose your password: ");
      break;
    case CREATE_PASSWORD:
      //add_user(m_name, cmd);
      m_state = AUTHENTICATED;
      sendFd(m_fd, "Welcome!!");
      break;
    default:
      std::cerr << "Unknown session state" << std::endl;
  }
  return true;
}

int Session::fd() const
{
  return m_fd;
}

std::uint64_t Session::uid() const
{
  return m_uid;
}

Session::State Session::state() const
{
  return m_state;
}

std::chrono::time_point<std::chrono::system_clock> Session::lastCmdTime() const
{
  return m_lastCmdTime;
}
} // namespace wyk
