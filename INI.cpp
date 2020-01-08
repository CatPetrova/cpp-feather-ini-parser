#include "stdAfx.h"
#include "INI.h"
#include <codecvt>
#include <locale>

#ifndef FINI_WIDE_SUPPORT
std::ostream &fini_err = std::cerr;
#else
std::wostream &fini_err = std::wcerr;
#endif

fini_string_t& l_trim(fini_string_t& str, const fini_string_t trim_chars = _T("\t\v\f; ")) {
  str.erase(0, str.find_first_not_of(trim_chars)); return str;
}

fini_string_t& r_trim(fini_string_t& str, const fini_string_t trim_chars = _T("\t\v\f; ")) {
  str.erase(str.find_last_not_of(trim_chars) + 1); return str;
}

fini_string_t& trim(fini_string_t& str, const fini_string_t trim_chars = _T("\t\v\f; ")) {
  return l_trim(r_trim(str, trim_chars), trim_chars);
}

template <typename T>
T convert_to(const fini_string_t &str) {
  fini_sstream_t ss(str);
  T num;
  ss >> num;
  return num;
}

template <>
fini_string_t convert_to<fini_string_t>(const fini_string_t &str) {
  return str;
}

template <>
const TCHAR* convert_to<const TCHAR*>(const fini_string_t &str) {
  return str.c_str();
}


/// Definition
INI::INI(const INI& from) : source(from.source), filename(from.filename) {
  // Deep clone INI
  for (auto i : from.sections) {
    select(i.first);
    for (auto j : *i.second)
      set(j.first, j.second);
  }
}

INI::INI(fini_string_t filename, bool doParse, int parseFlags) : source(SOURCE_FILE), filename(filename) {
  this->create(_T(""));

  if (doParse)
    parse(parseFlags);
}

INI:: ~INI() {
  clear();
}

void INI::clear() {
  for (std::pair<fini_string_t, keys_t*> section : sections) {
    delete section.second;
    section.second = nullptr;
  }
  sections.clear();
}

bool INI::parse(int parseFlags) {
  parseFlags = (parseFlags > 0) ? parseFlags : PARSE_FLAGS;

  switch (source)
  {
  case SOURCE_FILE: {
    fini_ifstream_t file(filename);
    std::locale loc(std::locale(), new std::codecvt_utf8_utf16<wchar_t>);
    file.imbue(loc);

    if (!file.is_open())
      return false;

    _parseFile(file, parseFlags);

    file.close();
  } break;

  case SOURCE_MEMORY:
    /*std::stringstream sstream;
    sstream.rdbuf()->pubsetbuf(data, dataSize);
    parse(sstream);*/
    break;
  }

  return true;
}

int INI::PARSE_FLAGS = 0, INI::SAVE_FLAGS = 0;

void INI::_parseFile(fini_ifstream_t& file, int parseFlags) {
  fini_string_t line;
  fini_string_t section; // Set default section (support for sectionless files)
  size_t i = 0;
  keys_t* current = this->current;

  while (std::getline(file, line)) {
    i++;

    // Parse comments
    if (parseFlags & PARSE_COMMENTS_SLASH || parseFlags & PARSE_COMMENTS_ALL)
      line = line.substr(0, line.find(_T("//")));
    if (parseFlags & PARSE_COMMENTS_HASH || parseFlags & PARSE_COMMENTS_ALL)
      line = line.substr(0, line.find(_T("#")));

    if (trim(line).size() == 0) // Skip empty lines
      continue;

    if (_T('[') == line.at(0)) { // Section
      section = trim(line, _T("[] ")); // Obtain key value, including contained spaced
      if (section.size() == 0) // If no section value, use default section
        current = this->current;

      if (sections.find(section) != sections.end()) {
        std::wcerr << _T("Error: cpp-feather-ini-parser: Duplicate section '") + section + _T("':") << i << std::endl;
        throw - 1;
      }

      current = new keys_t;
      sections[section] = current;
    }
    else {
      size_t indexEquals = line.find(_T("="));
      if (indexEquals != fini_string_t::npos) {
        fini_string_t key = line.substr(0, indexEquals), value = line.substr(indexEquals + 1);
        r_trim(key);
        l_trim(value);

        if ((*current).find(key) != (*current).end()) {
          fini_err << _T("Error: cpp-feather-ini-parser: Duplicate key '") + key + _T("':") << i << std::endl;
          throw - 1;
        }

        (*current).emplace(key, value);
      }
    }
  }
}

bool INI::save(fini_string_t filename, int saveFlags) {
  saveFlags = (saveFlags > 0) ? saveFlags : SAVE_FLAGS;

  fini_ofstream_t file((filename == _T("")) ? this->filename : filename, std::ios::trunc);
  if (!file.is_open())
    return false;

  // Save remaining sections
  for (auto i : sections) {
    //if (i.first == "")
    //  continue;
    if (saveFlags & SAVE_PRUNE && i.second->size() == 0)  // No keys/values in section, skip to next
      continue;

    // Write section
    if (i.first != _T("")) {
      if (saveFlags & SAVE_SPACE_SECTIONS)
        file << _T("[ ") << i.first << _T(" ]") << std::endl;
      else
        file << _T('[') << i.first << _T(']') << std::endl;
    }

    // Loop through key & values
    for (auto j : *i.second) {
      if (saveFlags & SAVE_PRUNE && j.second == _T(""))
        continue;

      // Write key & value
      if (saveFlags & SAVE_TAB_KEYS && i.first != _T(""))
        file << _T('\t'); // Insert indent

      if (saveFlags & SAVE_SPACE_KEYS)
        file << j.first << _T(" = ") << j.second;
      else
        file << j.first << _T('=') << j.second;

      if (saveFlags & SAVE_SEMICOLON_KEYS)
        file << _T(';');

      file << std::endl;
    }

    // New section line
    if (saveFlags & SAVE_PADDING_SECTIONS)
      file << _T('\n');
  }

  file.close();

  return true;
}

//Provide bracket access to section contents
INI::keys_t& INI::operator[](fini_string_t section) {
  select(section);
  return *current;
}

//Create a new section and select it
void INI::create(fini_string_t section) {
  if (section != _T("") && sections.find(section) != sections.end()) {
    fini_err << _T("Error: cpp-feather-ini-parser: Duplicate section '") << section << _T("'") << std::endl;
    throw - 1;
  }

  current = new keys_t;
  sections[section] = current;
}

//Removes a section including all key/value pairs
void INI::remove(fini_string_t section) {
  if (select(section, true))
    sections.erase(section);

  current = NULL;
}

//Select a section for performing operations
bool INI::select(fini_string_t section, bool noCreate) {
  sections_t::iterator sectionsit = sections.find(section);
  if (sectionsit == sections.end()) {
    if (!noCreate)
      create(section);

    return false;
  }

  current = sectionsit->second;
  return true;
}

fini_string_t INI::get(fini_string_t section, fini_string_t key, fini_string_t def) {
  select(section);
  return get(key, def);
}

fini_string_t INI::get(fini_string_t key, fini_string_t def) {
  auto it = current->find(key);
  if (it == current->end())
    return def;

  return it->second;
}

template<class T>
T INI::getAs(fini_string_t section, fini_string_t key, T def) {
  return getAs<T>(key, def);
}

template<class T>
T INI::getAs(fini_string_t key, T def) {
  auto it = current->find(key);
  if (it == current->end())
    return def;

  return convert_to<T>(it->second);
}

void INI::set(fini_string_t section, fini_string_t key, fini_string_t value) {
  if (!select(section))
    create(section);

  set(key, value);
}

void INI::set(fini_string_t key, fini_string_t value) {
  (*current)[key] = value;
}
