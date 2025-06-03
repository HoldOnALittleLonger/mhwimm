#ifndef _MHWIMMC_UI_H_
#define _MHWIMMC_UI_H_

#include <iostream>
#include <string>
#include <cstddef>

namespace mhwimmc_ui_ns {

  class mhwimmc_ui finally {
  public:
    mhwimmc_ui() :
      local_msg_buffer_("nil"),
      prompt_msg_("mhwimmc: "),
      startup_msg_("Monster Hunter World:Iceborne Mod Manager cmd tool"),
      space_indent_(8)
        {}

    mhwimmc_ui(const mhwimmc_ui &) =delete;
    mhwimmc_ui(mhwimmc_ui &&) =delete;
    mhwimmc_ui &operator=(const mhwimmc_ui &) =delete;
    mhwimmc_ui &operator=(mhwimmc_ui &&) =delete;

    /**
     * printPrompt - print prompt message on the console
     * # thread function should invokes this member method to tell user
     *   that next command cycle have been started
     */
    void printPrompt(void)
    {
      std::cout<< prompt_msg_ << flush;
    }
    
    /* printStartupMsg - print startup msg */
    void printStartupMsg(void)
    {
      printPrompt();
      std::cout << startup_msg_ << flush;
    }

    /**
     * printMessage - print message on the console
     * @msg:          referrence to std::string object which holds the
     *                message from Command Module
     */
    void printMessage(std::string &msg)
    {
      std::cout<< msg << flush;
    }

    /**
     * readFromUser - read user input from standard input stream
     * return:        size of characters this routine have been readed
     */
    ssize_t readFromUser(void);

    /**
     * sendCMDTo - send command readed from user to the destination
     * @des:       where to place
     */
    void sendCMDTo(std::string &des)
    {
      des = local_msg_buffer_;
    }

    /* printIndentSpaces - print indent */
    void printIndentSpaces(void)
    {
      auto x(space_indent_);
      do {
        std::cout.put(' ');
      } while (--x);
      std::cout << flush;
    }

    /* newLine - start a new line */
    void newLine(void)
    {
      std::cout << std::endl;
    }

  private:
    /* local_msg_buffer_ - temporary local message buffer */
    std::string local_msg_buffer_;
  
    /* prompt_msg_ - a constant object to stores prompt message */
    const std::string_view prompt_msg_;

    /* startup_msg_ - the message will be print when program is startup */
    const std::string_view startup_msg_;

    /* space_indent_ - number of spaces for line indent */
    const unsigned short space_indent_;
  };

}


#endif
