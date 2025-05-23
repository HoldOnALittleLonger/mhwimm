#ifndef _MHWIMMC_CONFIG_H_
#define _MHWIMMC_CONFIG_H_

#include <string>

template<typename _Tp>
struct config_struct_traits {
  typedef _Tp config_key_type;
  typedef _Tp config_val_type;
};

template<typename _Tp *>
struct config_struct_traits {
  typedef _Tp* config_key_type;
  typedef _Tp* config_val_type;
};

template<typename const _Tp *>
struct config_struct_traits {
  typedef const _Tp* config_key_type;
  typedef const _Tp* config_val_type;
};

template<typename _Tp>
struct config_struct : config_struct_traits<_Tp> {
  using key_type = config_key_type;
  using val_type = config_val_type;

  key_type userhome;
  key_type mhwiroot;
  key_type mhwimmcroot;
};

#endif
