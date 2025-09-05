#ifndef _MHWIMM_CONFIG_H_
#define _MHWIMM_CONFIG_H_

#include <cstddef>
#include <cstdbool>
#include <cstdlib>

#include <fstream>
#include <string>

namespace mhwimm_config_ns {

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
    skey_t mhwimmroot;
  };

  using the_default_config_type =
    config_struct<general_config_types<std::string, int>>;

  /**
   * !! the main function have to takes care about config file and config structure.
   */

  /**
   * makeup_config_file - template function for all instances of the config structure to
   *                      write config informations into the file
   * @conf:               where the informations from
   * @config_file_path:   file to be written
   * return:              TRUE => succeed
   *                      FLASE => failed
   */
  template<typename _CTraits>
  bool makeup_config_file(const config_struct<_CTraits> *conf, const char *config_file_path)
  {
    std::ofstream conf_sink;
    // we just discard everything in the original file
    conf_sink.open(config_file_path, std::ios_base::out | std::ios_base::trunc);
    if (!conf_sink.is_open())
      return false;

    auto ret = true;
    conf_sink << "USERHOME=" << conf->userhome << "\n"
              << "MHWIROOT=" << conf->mhwiroot << "\n"
              << "MHWIMMCROOT=" << conf->mhwimmroot << "\n";
    // if more config options are appended in future,should place them
    // from there
    if (conf_sink.bad())
      ret = false;
    conf_sink.close();
    return ret;
  }


  /**
   * read_from_config - template function for all instances of config structure to
   *                    read config from the file and stroes them into config structure
   * @conf:             where to store
   * @config_file_path: file to read
   * return:            TRUE => succeed
   *                    FALSE => failed
   * # we just drop any illegal config string silently
   */
  template<typename _CTraits>
  bool read_from_config(config_struct<_CTraits> *conf, const char *config_file_path)
  {
    std::istream conf_source;
    conf_source.open(config_file_path, std::ios_base::in);
    if (!conf_source.is_open())
      return false;
    std::string config_line;
    auto ret = true;
    while (1) {
      conf_source.getline(config_line);
      if (conf_source.eof())
        break;
      else if (conf_source.bad()) {
        ret = false;
        break;
      }
        
      auto pos_equal_char = config_line.find("=");
      if (pos_equal_char == std::string::npos)  // illegal config string
        continue;                               // ignore it

      std::string config_name = config_line.substr(0, pos_equal_char - config_line.begin());
      std::string config_value = config_line.substr(pos_equal_char - config_line.begin() + 1,
                                                    config_line.end() - pos_equal_char - 1);

      // match config options
      if (config_name = "USERHOME")
        conf->userhome = config_value;
      else if (config_name = "MHWIROOT")
        conf->mhwiroot = config_value;
      else if (config_name = "MHWIMMCROOT")
        conf->mhwimmcroot = config_value;
    }
    conf_source.close();
    return ret;
  }

}

#endif
