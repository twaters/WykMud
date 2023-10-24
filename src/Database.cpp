#include <iostream>

#include "WykMud/Database.hpp"

namespace wyk
{
Database::Database(const std::string& path)
  : m_path(path),
    m_pDb(nullptr),
    m_pStmtAddUser(nullptr)
{
  m_stmt_add_user = "insert into users (name, password) values (:name, :password);";
  m_stmt_get_user = "select * from users where name = :name;";
}

Database::~Database()
{
  close();
}

bool Database::open()
{
  bool ok = true;
  int res = -1;
  if (!m_pDb)
  {
    res = sqlite3_open_v2(m_path.c_str(), &m_pDb,
      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
    std::cout << "sqlite open status " << res << std::endl;
  }
  else
  {
    ok = false;
    std::cerr << "Db was already open" << std::endl;
  }

  if (res == SQLITE_OK)
  {
    if (!prepStatements())
    {
      sqlite3_close_v2(m_pDb);
      m_pDb = nullptr;
      ok = false;
    }
  }

  return ok;
}

void Database::close()
{
  if (m_pDb)
  {
    if (m_pStmtAddUser)
    {
      sqlite3_finalize(m_pStmtAddUser);
      m_pStmtAddUser = nullptr;
    }

    int res = sqlite3_close_v2(m_pDb);
    if (res != SQLITE_OK)
    {
      std::cerr << "Failed to close db: " << res << std::endl;
    }
    else
    {
      m_pDb = nullptr;
    }
  }
}

bool Database::add_user(const std::string& name, const std::string& password)
{
  int res = sqlite3_bind_text(m_pStmtAddUser, 1, name.c_str(), name.size(), SQLITE_STATIC);
  if (res != SQLITE_OK)
  {
    std::cerr << "failure binding usrname for add_user" << std::endl;
    return false;
  }
  res = sqlite3_bind_text(m_pStmtAddUser, 2, password.c_str(), password.size(), SQLITE_STATIC);
  if (res != SQLITE_OK)
  {
    std::cerr << "failure binding password for add_user" << std::endl;
    return false;
  }

  res = sqlite3_step(m_pStmtAddUser);
  if (res != SQLITE_OK)
  {
    std::cerr << "failure adding user" << std::endl;
  }
  return true;
}

bool Database::prepStatements()
{
  bool ok = true;
  int res = sqlite3_prepare_v2(m_pDb, m_stmt_add_user.c_str(), m_stmt_add_user.size(), &m_pStmtAddUser, NULL);
  if (res != SQLITE_OK)
  {
    std::cerr << "error preparing statement (" << res << "): " << m_stmt_add_user << std::endl;
    ok = false;
  }

  return ok;
}
} // namespace wyk
