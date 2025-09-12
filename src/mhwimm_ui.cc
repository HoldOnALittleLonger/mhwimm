/**
 * Member Method Definitions of mhwimm_ui
 */
#include "mhwimm_ui.h"

#include <cstring>

namespace mhwimm_ui_ns {

  static constinit std::size_t MAX_INPUT_LENGTH(256);

  /* readFromUser - wait user input */
  ssize_t mhwimm_ui::readFromUser(void)
  {
    char tmp_cbuf[MAX_INPUT_LENGTH] = {0};

  restart:
    std::cin.getline(tmp_cbuf, MAX_INPUT_LENGTH);
    std::size_t readed(std::cin.gcount());
    
    if (std::cin.fail() || readed < 0)
      return -1;

    if (!readed)
      goto restart;

    local_msg_buffer_ = tmp_cbuf;
    return readed;
  }
  
}
