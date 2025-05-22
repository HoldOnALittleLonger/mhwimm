#include "mhwimmc_ui.h"

namespace UI {

  ssize_t mhwimmc_ui::readFromUser(void)
  {
    std::cin >> local_msg_buffer_;
    if (std::cin.bad())
      return -1;
    return local_msg_buffer_.length();
  }
  
}
