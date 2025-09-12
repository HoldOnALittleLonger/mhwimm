/**
 * Monster Hunter World Iceborne Mod Manager Config
 * This file contains the detail about config options,
 * and some routines used to interactive with config
 * file.
 */
#ifndef _MHWIMM_CONFIG_H_
#define _MHWIMM_CONFIG_H_

#include <cstddef>
#include <cstdbool>
#include <cstdlib>

#include <fstream>
#include <string>

namespace mhwimm_config_ns {

  /**
   * template config_struct_traits - traits of config structure
   * @_MType:    template parameter which should a structure
   *             contained the real type for config options
   * @skey_t:    alias for string keyword type
   * @nkey_t:    alias for numberic keyword type
   */
  template<typename _MType>
  struct config_struct_traits {
    typedef typename _MType::config_string_key_type skey_t;
    typedef typename _MType::config_number_key_type nkey_t;
  };

  template<typename _MType>
  struct config_struct_traits<const _MType *> {
    typedef const _MType::config_string_key_type * skey_t;
    typedef const _MType::config_number_key_type * nkey_t;
  };

  template<typename _MType>
  struct config_struct_traits<_MType *> {
    typedef _MType::config_string_key_type * skey_t;
    typedef _MType::config_number_key_type * nkey_t;
  };

  /**
   * config_member_type - structure holds type definitions about
   *                      real config options
   * @config_string_key_type:    for string keyword type,it is
   *                             std::string
   * @config_number_key_type:    for numberic keyword type,it is
   *                             int
   */
  struct config_member_type {
    typedef std::string config_string_key_type;
    typedef int config_number_key_type;
  };

  /**
   * template config_struct - template structure for config
   * @_MType:    a type which contained the real type of configs
   * @type_tag:  identifier for @_MType
   * @skey_t:    string keyword type
   * @nkey_t:    numberic keyword type
   * @userhome:  current user's home,it usual is the env $HOME
   * @mhwiroot:  path to the root of mhwi install directory
   * @mhwimmroot:    path to the root of this application's configure
   *                 directory
   */
  template<typename _MType>
  struct config_struct {
    typedef _MType type_tag;
    typedef typename config_struct_traits<_MType>::skey_t skey_t;
    typedef typename config_struct_traits<_MType>::nkey_t nkey_t;
    skey_t userhome;
    skey_t mhwiroot;
    skey_t mhwimmroot;
  };

  /**
   * get_config_traits - shortcut to get the type of config options by
   *                     the config struct type
   */
  template<typename _ConfigType>
  struct get_config_traits {
    typedef typename config_struct_traits<typename _ConfigType::type_tag>::skey_t skey_t;
    typedef typename config_struct_traits<typename _ConfigType::type_tag>::nkey_t nkey_t;
  };

  /* config_t - alias for config_struct<config_member_type> instance */
  using config_t = config_struct<config_member_type>;

  /**
   * makeup_config_file - template function for all instances of the config structure to
   *                      write config informations into config file
   * @_ConfigType:        template parameter used to indicates what config struct type
   *                      it is
   * @conf:               where the config informations from
   * @config_file_path:   file to be written
   * return:              TRUE => succeed
   *                      FLASE => failed
   * # we just discard everything in the original file.
   *   maybe an unexpected exception interrupts writting,in that case,
   *   the file must been truncated.
   *   to prevent this,caller should take care of make a copy of original
   *   config file.
   */
  template<typename _ConfigType>
  bool makeup_config_file(const _ConfigType *conf, const char *config_file_path)
  {
    std::ofstream conf_sink;

    conf_sink.open(config_file_path, std::ios_base::out | std::ios_base::trunc);
    if (!conf_sink.is_open())
      return false;

    auto ret = true;
    conf_sink << "USERHOME=" << conf->userhome << "\n"
              << "MHWIROOT=" << conf->mhwiroot << "\n"
              << "MHWIMMROOT=" << conf->mhwimmroot << "\n";
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
   * @_ConfigType:      template parameter used to indicates what the type of config 
   *                    struct it is
   * @conf:             where to store
   * @config_file_path: file to read
   * return:            TRUE => succeed
   *                    FALSE => failed
   * # we just drop any illegal config string silently
   */
  template<typename _ConfigType>
  bool read_from_config(_ConfigType *conf, const char *config_file_path)
  {
    std::ifstream conf_source;

    conf_source.open(config_file_path, std::ios_base::in);
    if (!conf_source.is_open())
      return false;

    char config_buf[256] = {0}; // local array for getline
    auto ret = true;
    while (1) {
      conf_source.getline(config_buf, 256);
      std::size_t readed(conf_source.gcount());

      if (conf_source.eof())
        break;
      else if (conf_source.bad()) {
        ret = false;
        break;
      }
      else if (readed == 0) // ENTER Character
        continue;

      std::string config_line(config_buf);
        
      auto pos_equal_char = config_line.find("=");
      if (pos_equal_char == std::string::npos)  // illegal config string
        continue;                               // ignore it

      typename get_config_traits<_ConfigType>::skey_t config_name;
      typename get_config_traits<_ConfigType>::skey_t config_value;

      config_name = config_line.substr(0, pos_equal_char);
      config_value = config_line.substr(pos_equal_char + 1,
                                        config_line.length() - pos_equal_char - 1);
      // match config options
      // for unknown options,drop them silently.
      if (config_name == "USERHOME")
        conf->userhome = config_value;
      else if (config_name == "MHWIROOT")
        conf->mhwiroot = config_value;
      else if (config_name == "MHWIMMROOT")
        conf->mhwimmroot = config_value;
    }
    conf_source.close();
    return ret;
  }

}

#endif
