/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* Option Parsing Definitions                                                 *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

/////////
// String table

#ifdef OPTTABLE_STR_TABLE_CODE

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverlength-strings"
#endif
static constexpr char OptionStrTableStorage[] = 
  "\0"
  "-\0"
  "--\0"
  "kind\0"
  "<input>\0"
  "<unknown>\0"
  "--addresses\0"
  "--adjust-vma=\0"
  "--adjust-vma\0"
  "-a\0"
  "--basenames\0"
  "--build-id=\0"
  "--build-id\0"
  "--cache-size=\0"
  "--cache-size\0"
  "--color=\0"
  "--color\0"
  "-C\0"
  "--debug-file-directory=\0"
  "--debug-file-directory\0"
  "--debuginfod\0"
  "--default-arch=\0"
  "--default-arch\0"
  "-demangle=false\0"
  "-demangle=true\0"
  "--demangle\0"
  "--dia\0"
  "--dsym-hint=\0"
  "--dsym-hint\0"
  "--dwp=\0"
  "--dwp\0"
  "-e=\0"
  "--exe=\0"
  "--exe\0"
  "-e\0"
  "-f=\0"
  "--fallback-debug-path=\0"
  "--fallback-debug-path\0"
  "--filter-markup\0"
  "--functions=\0"
  "--functions\0"
  "-f\0"
  "--help\0"
  "-h\0"
  "--inlines\0"
  "--inlining=false\0"
  "--inlining=true\0"
  "--inlining\0"
  "-i\0"
  "--no-debuginfod\0"
  "--no-demangle\0"
  "--no-inlines\0"
  "--no-untag-addresses\0"
  "--obj=\0"
  "--obj\0"
  "--output-style=\0"
  "--output-style\0"
  "--pretty-print\0"
  "--print-address\0"
  "--print-source-context-lines=\0"
  "--print-source-context-lines\0"
  "-p\0"
  "--relative-address\0"
  "--relativenames\0"
  "--skip-line-zero\0"
  "-s\0"
  "--untag-addresses\0"
  "--verbose\0"
  "--version\0"
  "-v\0"
  ;
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

static constexpr llvm::StringTable OptionStrTable =
    OptionStrTableStorage;
#endif // OPTTABLE_STR_TABLE_CODE

/////////
// Prefixes

#ifdef OPTTABLE_PREFIXES_TABLE_CODE
static constexpr llvm::StringTable::Offset OptionPrefixesTable[] = {
  0 /* prefixes */,
  1 /* prefixes */, 1 /* '-' */,
  1 /* prefixes */, 3 /* '--' */
};
#endif // OPTTABLE_PREFIXES_TABLE_CODE

/////////
// Prefix Union

#ifdef OPTTABLE_PREFIXES_UNION_CODE
static constexpr llvm::StringTable::Offset OptionPrefixesUnion[] = {
  1 /* '-' */,   3 /* '--' */
};
#endif // OPTTABLE_PREFIXES_UNION_CODE

/////////
// ValuesCode

#ifdef OPTTABLE_VALUES_CODE
#endif
/////////
// Groups

#ifdef OPTION
OPTION(0 /* no prefixes */, 6 /* kind */, grp_mach_o, Group, INVALID, INVALID, nullptr, 0, 0, 0,
       "llvm-symbolizer Mach-O Specific Options", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)

//////////
// Options

OPTION(0, 11 /* <input> */, INPUT, Input, INVALID, INVALID, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(0, 19 /* <unknown> */, UNKNOWN, Unknown, INVALID, INVALID, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 29 /* --addresses */, addresses, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Show address before line information", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 41 /* --adjust-vma= */, adjust_vma_EQ, Joined, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Add specified offset to object file addresses", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<offset>", nullptr)
OPTION(3, 55 /* --adjust-vma */, anonymous_0, Separate, INVALID, adjust_vma_EQ, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<offset>", nullptr)
OPTION(1, 68 /* -a */, anonymous_11, Flag, INVALID, addresses, nullptr, 0, DefaultVis, 0,
       "Alias for --addresses", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 71 /* --basenames */, basenames, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Strip directory names from paths", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 83 /* --build-id= */, build_id_EQ, Joined, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Build ID used to look up the object file", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 95 /* --build-id */, anonymous_1, Separate, INVALID, build_id_EQ, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 106 /* --cache-size= */, cache_size_EQ, Joined, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Max size in bytes of the in-memory binary cache.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 120 /* --cache-size */, anonymous_2, Separate, INVALID, cache_size_EQ, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 133 /* --color= */, color_EQ, Joined, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Whether to use color when symbolizing log markup: always, auto, never", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, "always,auto,never")
OPTION(3, 142 /* --color */, color, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Use color when symbolizing log markup.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 150 /* -C */, anonymous_13, Flag, INVALID, demangle, nullptr, 0, DefaultVis, 0,
       "Alias for --demangle", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 153 /* --debug-file-directory= */, debug_file_directory_EQ, Joined, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Path to directory where to look for debug files", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<dir>", nullptr)
OPTION(3, 177 /* --debug-file-directory */, anonymous_3, Separate, INVALID, debug_file_directory_EQ, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<dir>", nullptr)
OPTION(3, 200 /* --debuginfod */, debuginfod, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Use debuginfod to find debug binaries", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 213 /* --default-arch= */, default_arch_EQ, Joined, grp_mach_o, INVALID, nullptr, 0, DefaultVis, 0,
       "Default architecture (for multi-arch objects)", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 229 /* --default-arch */, anonymous_4, Separate, grp_mach_o, default_arch_EQ, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 244 /* -demangle=false */, anonymous_29, Flag, INVALID, no_demangle, nullptr, 0, DefaultVis, 0,
       "Alias for --no-demangle", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 260 /* -demangle=true */, anonymous_28, Flag, INVALID, demangle, nullptr, 0, DefaultVis, 0,
       "Alias for --demangle", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 275 /* --demangle */, demangle, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Demangle function names", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 286 /* --dia */, use_dia, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Use the DIA library to access symbols (Windows only)", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 292 /* --dsym-hint= */, dsym_hint_EQ, Joined, grp_mach_o, INVALID, nullptr, 0, DefaultVis, 0,
       "Path to .dSYM bundles to search for debug info for the object files", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<dir>", nullptr)
OPTION(3, 305 /* --dsym-hint */, anonymous_6, Separate, grp_mach_o, dsym_hint_EQ, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<dir>", nullptr)
OPTION(3, 317 /* --dwp= */, dwp_EQ, Joined, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Path to DWP file to be use for any split CUs", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<file>", nullptr)
OPTION(3, 324 /* --dwp */, anonymous_5, Separate, INVALID, dwp_EQ, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<file>", nullptr)
OPTION(1, 330 /* -e= */, anonymous_17, Joined, INVALID, obj_EQ, nullptr, 0, DefaultVis, 0,
       "Alias for --obj", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<file>", nullptr)
OPTION(3, 334 /* --exe= */, anonymous_14, Joined, INVALID, obj_EQ, nullptr, 0, DefaultVis, 0,
       "Alias for --obj", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<file>", nullptr)
OPTION(3, 341 /* --exe */, anonymous_15, Separate, INVALID, obj_EQ, nullptr, 0, DefaultVis, 0,
       "Alias for --obj", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<file>", nullptr)
OPTION(1, 347 /* -e */, anonymous_16, JoinedOrSeparate, INVALID, obj_EQ, nullptr, 0, DefaultVis, 0,
       "Alias for --obj", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<file>", nullptr)
OPTION(1, 350 /* -f= */, anonymous_19, Joined, INVALID, functions_EQ, nullptr, 0, DefaultVis, 0,
       "Alias for --functions=", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 354 /* --fallback-debug-path= */, fallback_debug_path_EQ, Joined, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Fallback path for debug binaries", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<dir>", nullptr)
OPTION(3, 377 /* --fallback-debug-path */, anonymous_7, Separate, INVALID, fallback_debug_path_EQ, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<dir>", nullptr)
OPTION(3, 399 /* --filter-markup */, filter_markup, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Filter symbolizer markup from stdin.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 415 /* --functions= */, functions_EQ, Joined, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Print function name for a given address", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, "none,short,linkage")
OPTION(3, 428 /* --functions */, functions, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Print function name for a given address", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 440 /* -f */, anonymous_18, Flag, INVALID, functions, nullptr, 0, DefaultVis, 0,
       "Alias for --functions", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 443 /* --help */, help, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display this help", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 450 /* -h */, anonymous_20, Flag, INVALID, help, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 453 /* --inlines */, inlines, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Print all inlined frames for a given address", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 463 /* --inlining=false */, anonymous_27, Flag, INVALID, no_inlines, nullptr, 0, DefaultVis, 0,
       "Alias for --no-inlines", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 480 /* --inlining=true */, anonymous_26, Flag, INVALID, inlines, nullptr, 0, DefaultVis, 0,
       "Alias for --inlines", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 496 /* --inlining */, anonymous_22, Flag, INVALID, inlines, nullptr, 0, DefaultVis, 0,
       "Alias for --inlines", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 507 /* -i */, anonymous_21, Flag, INVALID, inlines, nullptr, 0, DefaultVis, 0,
       "Alias for --inlines", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 510 /* --no-debuginfod */, no_debuginfod, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Don't use debuginfod to find debug binaries", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 526 /* --no-demangle */, no_demangle, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Don't demangle function names", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 540 /* --no-inlines */, no_inlines, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Do not print inlined frames", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 553 /* --no-untag-addresses */, no_untag_addresses, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Remove memory tags from addresses before symbolization", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 574 /* --obj= */, obj_EQ, Joined, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Path to object file to be symbolized (if not provided, object file should be specified for each input line)", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<file>", nullptr)
OPTION(3, 581 /* --obj */, anonymous_8, Separate, INVALID, obj_EQ, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<file>", nullptr)
OPTION(3, 587 /* --output-style= */, output_style_EQ, Joined, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Specify print style. Supported styles: LLVM, GNU, JSON", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "style", "LLVM,GNU,JSON")
OPTION(3, 603 /* --output-style */, anonymous_9, Separate, INVALID, output_style_EQ, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "style", "LLVM,GNU,JSON")
OPTION(3, 618 /* --pretty-print */, pretty_print, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Make the output more human friendly", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 633 /* --print-address */, anonymous_12, Flag, INVALID, addresses, nullptr, 0, DefaultVis, 0,
       "Alias for --addresses", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 649 /* --print-source-context-lines= */, print_source_context_lines_EQ, Joined, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Print N lines of source file context", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 679 /* --print-source-context-lines */, anonymous_10, Separate, INVALID, print_source_context_lines_EQ, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 708 /* -p */, anonymous_23, Flag, INVALID, pretty_print, nullptr, 0, DefaultVis, 0,
       "Alias for --pretty-print", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 711 /* --relative-address */, relative_address, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Interpret addresses as addresses relative to the image base", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 730 /* --relativenames */, relativenames, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Strip the compilation directory from paths", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 746 /* --skip-line-zero */, skip_line_zero, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "If an address does not have an associated line number, use the last line number from the current sequence in the line-table", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 763 /* -s */, anonymous_24, Flag, INVALID, basenames, nullptr, 0, DefaultVis, 0,
       "Alias for --basenames", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 766 /* --untag-addresses */, untag_addresses, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 784 /* --verbose */, verbose, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Print verbose line info", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 794 /* --version */, version, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display the version", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 804 /* -v */, anonymous_25, Flag, INVALID, version, nullptr, 0, DefaultVis, 0,
       "Alias for --version", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
#endif // OPTION

#ifdef SIMPLE_ENUM_VALUE_TABLE

struct SimpleEnumValue {
  const char *Name;
  unsigned Value;
};

struct SimpleEnumValueTable {
  const SimpleEnumValue *Table;
  unsigned Size;
};
static const SimpleEnumValueTable SimpleEnumValueTables[] = {};
static const unsigned SimpleEnumValueTablesSize = std::size(SimpleEnumValueTables);
#endif // SIMPLE_ENUM_VALUE_TABLE


