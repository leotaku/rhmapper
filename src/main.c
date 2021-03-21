#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

#include "csv.c"

#define BBTEX_VERSION_MAJOR "0"
#define BBTEX_VERSION_MINOR "1"
#define BBTEX_VERSION BBTEX_VERSION_MAJOR "." BBTEX_VERSION_MINOR

enum format { f_undefined, f_bibtex, f_csv };
typedef enum format format_t;

struct options {
  format_t source;
  format_t target;
};
typedef struct options options_t;

format_t formatparse(const char * name) {
  if (!strcmp(name, "csv") || !strcmp(name, "CSV")) {
    return f_csv;
  } else if (!strcmp(name, "bib") || !strcmp(name, "BIB")) {
    return f_bibtex;
  } else if (!strcmp(name, "bibtex") || !strcmp(name, "BIBTEX") || !strcmp(name, "BibTeX")) {
    return f_bibtex;
  } else {
    return f_undefined;
  }
}

void error(const char * format, ...) {
  va_list arg;

  fprintf(stderr, "Error: ");
  va_start(arg, format);
  vfprintf(stderr, format, arg);
  va_end(arg);
  fprintf(stderr, "\n");

  exit(1);
}

void help() {
  puts("Usage: bbtex [OPTION]...");
  puts("Convert between CSV and BibTeX formats.");
  puts("Reads from STDIN and writes to STDOUT.");
  puts("");
  puts("Options:");
  puts("  -t, --target  Target format");
  puts("  -s, --source  Source format");
  puts("");
  puts("Supported formats:");
  puts("  CSV:    https://tools.ietf.org/html/rfc4180/");
  puts("  BibTeX: http://www.bibtex.org/Format/");
  puts("");
}

options_t optparse(int argc, char *argv[]) {
  options_t opts;
  char c;
  opterr = false;
  for (size_t i = 0; i < argc; i++) {
    if (!strcmp(argv[i], "--help")) {
      argv[i] = "-h";
    } else if (!strcmp(argv[i], "--version")) {
      argv[i] = "-v";
    } else if (!strcmp(argv[i], "--source")) {
      argv[i] = "-s";
    } else if (!strcmp(argv[i], "--target")) {
      argv[i] = "-t";
    } else if (!strncmp(argv[i], "--", 2)) {
      error("invalid long option '%s'", argv[i]);
    }
  }

  while ((c = getopt(argc, argv, "hvs:t:")) != -1) {
    switch (c) {
    case 'h':
      help();
      exit(0);
    case 'v':
      puts("bbtex version " BBTEX_VERSION);
      exit(0);
    case 's':
      opts.source = formatparse(optarg);
      if (opts.source == 0) {
        error("invalid source format '%s'", optarg);
      }
      break;
    case 't':
      opts.target = formatparse(optarg);
      if (opts.source == 0) {
        error("invalid target format '%s'", optarg);
      }
      break;
    default:
      error("invalid option '-%c'", optopt);
    }
  }

  return opts;
}

int main(int argc, char *argv[]) {
  options_t opts = optparse(argc, argv);

  if (argc == 1) {
    csv_parse(stdin);
  } else {
    FILE *fp = fopen(argv[1], "r");
    if(!fp) {
        perror("File opening failed");
        return EXIT_FAILURE;
    }

    csv_parse(fp);
  }
}
