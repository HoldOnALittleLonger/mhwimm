#ifndef _MHWIMMC_CONFIG_H_
#define _MHWIMMC_CONFIG_H_

#include <cstddef>

namespace mhwimmc_config_ns {

  template<typename _StrKeyTp, typename _NumKeyTp>
  struct general_config_types {
    typedef _StrKeyTp str_key;
    typedef _NumKeyTp num_key;
  };

  template<typename _Tp>
  struct config_struct_traits {
    typedef _Tp::str_key skey_t;
    typedef _Tp::num_key nkey_t;
  };

  template<typename _Tp *>
  struct config_struct_traits {
    typedef _Tp::str_key * skey_t;
    typedef _Tp::num_key * nkey_t;
  };

  template<typename const _Tp *>
  struct config_struct_traits {
    typedef const _Tp::str_key * skey_t;
    typedef const _Tp::num_key * nkey_t;
  };

  template<typename _Tp>
  struct config_struct : config_struct_traits<_Tp> {
    skey_t userhome;
    skey_t mhwiroot;
    skey_t mhwimmcroot;
  };

  using the_default_config_type =
    config_struct<general_config_types<std::string, int>>;

}

#endif
