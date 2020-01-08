#ifndef HENGSHUI2C_HENGSHUI_INI_H_
#define HENGSHUI2C_HENGSHUI_INI_H_

#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <vector>
#include <iostream>

#ifdef _UNICODE
#define FINI_WIDE_SUPPORT
#endif

#ifndef FINI_WIDE_SUPPORT
  typedef std::stringstream fini_sstream_t;
  typedef std::string fini_string_t;
  typedef char fini_char_t;
  typedef std::ifstream fini_ifstream_t;
  typedef std::ofstream fini_ofstream_t;
#else
  typedef std::wstringstream fini_sstream_t;
  typedef std::wstring fini_string_t;
  typedef wchar_t fini_char_t;
  typedef std::wifstream fini_ifstream_t;
  typedef std::wofstream fini_ofstream_t;
#endif


///
class INI
{
public:
/// Define
  static int PARSE_FLAGS, SAVE_FLAGS;

  typedef fini_char_t data_t;

  typedef typename std::map<fini_string_t, fini_string_t> keys_t;
  typedef typename std::map<fini_string_t, keys_t*> sections_t;
  typedef typename keys_t::value_type keyspair_t;
  typedef typename sections_t::value_type sectionspair_t;

  enum source_e {SOURCE_FILE, SOURCE_MEMORY};
  enum save_e {SAVE_PRUNE = 1 << 0, SAVE_PADDING_SECTIONS = 1 << 1, SAVE_SPACE_SECTIONS = 1 << 2, SAVE_SPACE_KEYS  = 1 << 3, SAVE_TAB_KEYS  = 1 << 4, SAVE_SEMICOLON_KEYS  = 1 << 5};
  enum parse_e {PARSE_COMMENTS_SLASH = 1 << 0, PARSE_COMMENTS_HASH = 1 << 1, PARSE_COMMENTS_ALL = 1 << 2};

/// Data
   const source_e source;
   const fini_string_t filename;
   //data_t* data;
   //size_t dataSize;

   keys_t* current;
   sections_t sections;

/// Methods
  INI(const INI& from);
  INI(fini_string_t filename, bool doParse, int parseFlags = 0);
  //INI(void* data, size_t dataSize, bool doParse)
  ~INI();
  void clear();
  bool parse(int parseFlags = 0);
  void _parseFile(fini_ifstream_t& file, int parseFlags);
  bool save(fini_string_t filename, int saveFlags = 0);

  keys_t& operator[](fini_string_t section);
  void create(fini_string_t section);
  void remove(fini_string_t section);
  bool select(fini_string_t section, bool noCreate = false);
  fini_string_t get(fini_string_t section, fini_string_t key, fini_string_t def);
  fini_string_t get(fini_string_t key, fini_string_t def);
    template<class T>
  T getAs(fini_string_t section, fini_string_t key, T def = T());
    template<class T>
  T getAs(fini_string_t key, T def = T());
  void set(fini_string_t section, fini_string_t key, fini_string_t value);
  void set(fini_string_t key, fini_string_t value);
};

#endif