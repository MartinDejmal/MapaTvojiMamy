#include "LocationRegistry.h"

namespace {
struct LocationItem {
  const char* key;
  int ledIndex;
};

const LocationItem kLocations[] = {
    {"benesov", 0},       {"beroun", 1},        {"blansko", 2},
    {"breclav", 3},       {"brno-mesto", 4},    {"brno-venkov", 5},
    {"bruntal", 6},       {"ceska-lipa", 7},    {"ceske-budejovice", 8},
    {"cesky-krumlov", 9}, {"cheb", 10},         {"chomutov", 11},
    {"chrudim", 12},      {"decin", 13},        {"domazlice", 14},
    {"frydek-mistek", 15},{"havlickuv-brod",16},{"hodonin", 17},
    {"hradec-kralove",18},{"jablonec-nad-nisou",19}, {"jesenik", 20},
    {"jicin", 21},        {"jihlava", 22},      {"jindrichuv-hradec", 23},
    {"karlovy-vary", 24}, {"karvina", 25},      {"kladno", 26},
    {"klatovy", 27},      {"kolin", 28},        {"kromeriz", 29},
    {"kutna-hora", 30},   {"liberec", 31},      {"litomerice", 32},
    {"louny", 33},        {"melnik", 34},       {"mlada-boleslav", 35},
    {"most", 36},         {"nachod", 37},       {"novy-jicin", 38},
    {"nymburk", 39},      {"olomouc", 40},      {"opava", 41},
    {"ostrava-mesto", 42},{"pardubice", 43},    {"pelhrimov", 44},
    {"pisek", 45},        {"plzen-mesto", 46},  {"plzen-jih", 47},
    {"plzen-sever", 48},  {"prachatice", 49},   {"praha-vychod", 50},
    {"praha-zapad", 51},  {"prerov", 52},       {"pribram", 53},
    {"prostejov", 54},    {"rakovnik", 55},     {"rokycany", 56},
    {"rychnov-nad-kneznou", 57}, {"semily", 58},{"sokolov", 59},
    {"strakonice", 60},   {"sumperk", 61},      {"svitavy", 62},
    {"tabor", 63},        {"tachov", 64},       {"teplice", 65},
    {"trebic", 66},       {"trutnov", 67},      {"uherske-hradiste", 68},
    {"usti-nad-labem",69},{"usti-nad-orlici",70},{"vsetin", 71},
    {"vyskov", 72},       {"zlin", 73},         {"znojmo", 74},
    {"zdar-nad-sazavou",75}, {"prague", 76},
};

}  // namespace

String LocationRegistry::normalizeKey(const String& key) const {
  String normalized = key;
  normalized.trim();
  normalized.toLowerCase();

  normalized.replace("á", "a");
  normalized.replace("ä", "a");
  normalized.replace("č", "c");
  normalized.replace("ď", "d");
  normalized.replace("é", "e");
  normalized.replace("ě", "e");
  normalized.replace("í", "i");
  normalized.replace("ň", "n");
  normalized.replace("ó", "o");
  normalized.replace("ř", "r");
  normalized.replace("š", "s");
  normalized.replace("ť", "t");
  normalized.replace("ú", "u");
  normalized.replace("ů", "u");
  normalized.replace("ý", "y");
  normalized.replace("ž", "z");

  normalized.replace(" ", "-");
  normalized.replace("_", "-");

  return normalized;
}

int LocationRegistry::findLedIndexByKey(const String& key) const {
  const String normalized = normalizeKey(key);

  for (size_t i = 0; i < (sizeof(kLocations) / sizeof(kLocations[0])); ++i) {
    if (normalized == kLocations[i].key) {
      return kLocations[i].ledIndex;
    }
  }

  return -1;
}
