#include <iostream>
#include <memory>

#include <signal.h>
#include <unistd.h>

#include "WykMud/Database.hpp"
#include "WykMud/Server.hpp"

wyk::Server* g_pServer;

void sig_handler(int sig)
{
  std::cout << "Shutting down... (sig " << sig << ")" << std::endl;
  g_pServer->close();
}

int main(int argc, char** argv)
{
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  std::string dbPath("/home/twaters/projects/WykMud/config/db.sqlite");
  wyk::Database db(dbPath);
  if (!db.open())
  {
    return -1;
  }

  std::uint16_t port(4000);
  g_pServer = new wyk::Server(port, 3);
  if (g_pServer->open())
  {
    std::cout << "Opened server on port " << port << std::endl;
    g_pServer->listen();
  }
  else
  {
    return -1;
  }

  delete g_pServer;
}
