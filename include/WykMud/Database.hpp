#include <string>

#include "sqlite/sqlite3.h"

namespace wyk
{
class Database
{
  public:
    explicit Database(const std::string& path);
    ~Database();

    bool open();
    void close();

    bool add_user(const std::string& username, const std::string& password);
  private:
    bool prepStatements();

    std::string m_path;
    sqlite3* m_pDb;

    std::string m_stmt_add_user;
    std::string m_stmt_get_user;

    sqlite3_stmt* m_pStmtAddUser;

    /// @cond
    Database(const Database&) = delete;
    Database(Database&&) = delete;
    Database& operator=(const Database&) = delete;
    Database& operator=(Database&&) = delete;
    /// @endcond
};
} // namespace wyk
