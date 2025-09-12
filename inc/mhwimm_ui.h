/**
 * Monster Hunter World Iceborne Mod Manager User Interface
 * This file contains UI definition.
 */
#ifndef _MHWIMM_UI_H_
#define _MHWIMM_UI_H_

#include <iostream>
#include <string>
#include <cstddef>

namespace mhwimm_ui_ns {

  /**
   * mhwimm_ui - UI of mhwimm,wait user input,pass command to
   *             Executor,retrieve output from Executor and
   *             show the messages to user
   */
  class mhwimm_ui final {
  public:
    mhwimm_ui()
      : local_msg_buffer_("nil"),
        prompt_msg_("mhwimm: "),
        startup_msg_("Monster Hunter World:Iceborne Mod Manager cmd tool"),
        space_indent_(8)
    {}

    // no destructor because this class has not dynamically allocating.

    // disabled copying,moving.
    mhwimm_ui(const mhwimm_ui &) =delete;
    mhwimm_ui(mhwimm_ui &&) =delete;
    mhwimm_ui &operator=(const mhwimm_ui &) =delete;
    mhwimm_ui &operator=(mhwimm_ui &&) =delete;

    /**
     * printPrompt - print prompt message on the console
     * # thread function should invokes this member method to tell user
     *   that next command cycle have been started
     */
    void printPrompt(void)
    {
      std::cout << prompt_msg_ << std::flush;
    }
    
    /* printStartupMsg - print startup msg */
    void printStartupMsg(void)
    {
      printPrompt();
      std::cout << startup_msg_ << std::flush;
    }

    /**
     * printMessage - print message on the console
     * @msg:          referrence to std::string object which holds the
     *                message from Command Module
     */
    void printMessage(const std::string &msg)
    {
      std::cout << msg << std::flush;
    }

    /**
     * readFromUser - read user input from standard input stream
     * return:        size of characters this routine have been readed
     */
    ssize_t readFromUser(void);

    /**
     * sendCMDTo - send command input from user to the destination
     * @des:       where to place
     */
    void sendCMDTo(std::string &des)
    {
      des = local_msg_buffer_;
    }

    /* printIndentSpaces - print indent */
    void printIndentSpaces(void)
    {
      char indent[space_indent_ + 1] = {0};
      for (unsigned short i(0); i < space_indent_; ++i)
        indent[i] = ' ';
      std::cout << indent << std::flush;
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
