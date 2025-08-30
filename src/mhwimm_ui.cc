#include "mhwimm_ui.h"

namespace mhwimm_ui_ns {

  ssize_t mhwimm_ui::readFromUser(void)
  {
    std::cin >> local_msg_buffer_;
    if (std::cin.bad())
      return -1;
    return local_msg_buffer_.length();
  }
  
}
