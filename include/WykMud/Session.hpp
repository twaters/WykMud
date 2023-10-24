#include <chrono>
#include <cstdint>

namespace wyk
{
class Session
{
  public:
    enum State : std::uint16_t
    {
      CLOSED,
      REQ_USERNAME,
      AUTHENTICATE_USER,
      CREATE_USER,
      CREATE_PASSWORD,
      AUTHENTICATED,
      INACTIVE
    };

    explicit Session(int fd);
    ~Session();
    virtual bool processCommand(const std::string& cmd);

    int fd() const;
    std::uint64_t uid() const;
    State state() const;
    std::chrono::time_point<std::chrono::system_clock> lastCmdTime() const;

  private:
    int m_fd{-1};                             //!< Session socket file descriptor
    std::uint64_t m_uid {0};                  //!< Authenticated user ID
    State m_state{State::CLOSED};             //!< Session state
    std::chrono::time_point<std::chrono::system_clock> m_lastCmdTime;  //!< Time of last received command

    std::string m_name;

    /// @cond
    Session(const Session&) = delete;
    Session(Session&&) = delete;
    Session& operator=(const Session&) = delete;
    Session& operator=(Session&&) = delete;
    /// @endcond
};
} // namespace wyk
