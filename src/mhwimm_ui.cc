#include "mhwimm_ui.h"

#include <cstring>

namespace mhwimm_ui_ns {

  static constinit std::size_t MAX_INPUT_LENGTH(256);

  ssize_t mhwimm_ui::readFromUser(void)
  {
    char tmp_cbuf[MAX_INPUT_LENGTH] = {0};
    std::cin.getline(tmp_cbuf, MAX_INPUT_LENGTH);
    
    if (std::cin.fail())
      return -1;

    std::size_t readed(std::cin.gcount());
    if (!readed)
      return 0;

    local_msg_buffer_ = tmp_cbuf;
    return readed;
  }
  
}
