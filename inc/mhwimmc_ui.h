#ifndef _MHWIMMC_UI_H_
#define _MHWIMMC_UI_H_

#include <string>
#include <cstddef>

class mhwimmc_ui finally {
public:
  mhwimmc_ui() : local_msg_buffer_("nil"), prompt_msg_("mhwimmc: ") {}

  mhwimmc_ui(const mhwimmc_ui &) =delete;
  mhwimmc_ui(mhwimmc_ui &&) =delete;
  mhwimmc_ui &operator=(const mhwimmc_ui &) =delete;
  mhwimmc_ui &operator=(mhwimmc_ui &&) =delete;

  /**
   * printPrompt - print prompt message on the console
   * # thread function should invokes this member method to tell user
   *   that next command cycle have been started
   */
  void printPrompt(void);

  /**
   * printMessage - print message on the console
   * @msg:          referrence to std::string object which holds the
   *                message from Command Module
   */
  void printMessage(std::string &msg);

  /**
   * readFromUser - read user input from standard input stream
   * return:        size of characters this routine have been readed
   */
  ssize_t readFromUser(void);

  /**
   * sendCMDTo - send command readed from user to the destination
   * @des:       where to place
   * return:     size of characters have been sent
   */
  ssize_t sendCMDTo(std::string &des);
private:
  /* local_msg_buffer_ - temporary local message buffer */
  std::string_view local_msg_buffer_;
  
  /* prompt_msg_ - a constant object to stores prompt message */
  const std::string_view prompt_msg_;
};


#endif
