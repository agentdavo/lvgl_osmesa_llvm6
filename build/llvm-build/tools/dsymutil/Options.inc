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
  "Dsymutil\0"
  "<input>\0"
  "<unknown>\0"
  "--accelerator=\0"
  "--accelerator\0"
  "--arch=\0"
  "--arch\0"
  "--build-variant-suffix=\0"
  "--build-variant-suffix\0"
  "--dump-debug-map\0"
  "-D\0"
  "--fat64\0"
  "--flat\0"
  "-f\0"
  "--gen-reproducer\0"
  "--help\0"
  "-h\0"
  "-j\0"
  "--keep-function-for-static\0"
  "--linker=\0"
  "--linker\0"
  "--no-object-timestamp\0"
  "--no-odr\0"
  "--no-output\0"
  "--no-swiftmodule-timestamp\0"
  "--num-threads\0"
  "-o=\0"
  "--object-prefix-map=\0"
  "--object-prefix-map\0"
  "--oso-prepend-path=\0"
  "--oso-prepend-path\0"
  "--out=\0"
  "--out\0"
  "-o\0"
  "--quiet\0"
  "-q\0"
  "--remarks-drop-without-debug\0"
  "--remarks-output-format=\0"
  "--remarks-output-format\0"
  "--remarks-prepend-path=\0"
  "--remarks-prepend-path\0"
  "--reproducer=\0"
  "--reproducer\0"
  "--statistics\0"
  "--symtab\0"
  "-S\0"
  "-s\0"
  "--toolchain\0"
  "--update\0"
  "--use-reproducer=\0"
  "--use-reproducer\0"
  "-u\0"
  "--verbose\0"
  "--verify-dwarf=\0"
  "--verify-dwarf\0"
  "--verify\0"
  "--version\0"
  "-v\0"
  "-y\0"
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
  2 /* prefixes */, 1 /* '-' */, 3 /* '--' */,
  2 /* prefixes */, 3 /* '--' */, 1 /* '-' */
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
OPTION(0 /* no prefixes */, 6 /* Dsymutil */, grp_general, Group, INVALID, INVALID, nullptr, 0, 0, 0,
       "Dsymutil Options", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)

//////////
// Options

OPTION(0, 15 /* <input> */, INPUT, Input, INVALID, INVALID, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(0, 23 /* <unknown> */, UNKNOWN, Unknown, INVALID, INVALID, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 33 /* --accelerator= */, anonymous_13, Joined, INVALID, accelerator, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 48 /* --accelerator */, accelerator, Separate, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Specify the desired type of accelerator table. Valid options are 'Apple' (.apple_names, .apple_namespaces, .apple_types, .apple_objc), 'Dwarf' (.debug_names), 'Pub' (.debug_pubnames, .debug_pubtypes), 'Default' and 'None'", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<accelerator type>", nullptr)
OPTION(6, 62 /* --arch= */, anonymous_12, Joined, INVALID, arch, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 70 /* --arch */, arch, Separate, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Link DWARF debug information only for specified CPU architecture types. This option can be specified multiple times, once for each desired architecture. All CPU architectures will be linked by default.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<arch>", nullptr)
OPTION(6, 77 /* --build-variant-suffix= */, anonymous_20, Joined, INVALID, build_variant_suffix, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 101 /* --build-variant-suffix */, build_variant_suffix, Separate, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Specify the build variant suffix used to build the executable file.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<suffix=buildvariant>", nullptr)
OPTION(6, 124 /* --dump-debug-map */, dump_debug_map, Flag, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Parse and dump the debug map to standard output. No DWARF link will take place.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 141 /* -D */, dsym_search_path, Separate, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Specify a directory that contain dSYM files to search for.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<path>", nullptr)
OPTION(6, 144 /* --fat64 */, fat64, Flag, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Use a 64-bit header when emitting universal binaries.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 152 /* --flat */, flat, Flag, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Produce a flat dSYM file (not a bundle).", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 159 /* -f */, anonymous_5, Flag, grp_general, flat, nullptr, 0, DefaultVis, 0,
       "Alias for --flat", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 162 /* --gen-reproducer */, gen_reproducer, Flag, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Generate a reproducer consisting of the input object files. Alias for --reproducer=GenerateOnExit.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 179 /* --help */, help, Flag, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Prints this help output.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 186 /* -h */, anonymous_0, Flag, grp_general, help, nullptr, 0, DefaultVis, 0,
       "Alias for --help", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 189 /* -j */, anonymous_14, Separate, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Alias for --num-threads", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<threads>", nullptr)
OPTION(6, 192 /* --keep-function-for-static */, keep_func_for_static, Flag, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Make a static variable keep the enclosing function even if it would have been omitted otherwise.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 219 /* --linker= */, anonymous_19, Joined, INVALID, linker, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 229 /* --linker */, linker, Separate, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Specify the desired type of DWARF linker. Defaults to 'classic'", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<DWARF linker type>", nullptr)
OPTION(6, 238 /* --no-object-timestamp */, no_object_timestamp, Flag, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Don't check timestamp for object files.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 260 /* --no-odr */, no_odr, Flag, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Do not use ODR (One Definition Rule) for type uniquing.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 269 /* --no-output */, no_output, Flag, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Do the link in memory, but do not emit the result file.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 281 /* --no-swiftmodule-timestamp */, no_swiftmodule_timestamp, Flag, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Don't check timestamp for swiftmodule files.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 308 /* --num-threads */, threads, Separate, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Specifies the maximum number of simultaneous threads to use when linking multiple architectures.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<threads>", nullptr)
OPTION(3, 322 /* -o= */, anonymous_9, Joined, INVALID, output, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 326 /* --object-prefix-map= */, anonymous_11, Joined, INVALID, object_prefix_map, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 347 /* --object-prefix-map */, object_prefix_map, Separate, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Remap object file paths (but no source paths) before processing. Use this for Clang objects where the module cache location was remapped using -fdebug-prefix-map; to help dsymutil find the Clang module cache.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<prefix=remapped>", nullptr)
OPTION(6, 367 /* --oso-prepend-path= */, anonymous_10, Joined, INVALID, oso_prepend_path, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 387 /* --oso-prepend-path */, oso_prepend_path, Separate, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Specify a directory to prepend to the paths of object files.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<path>", nullptr)
OPTION(6, 406 /* --out= */, anonymous_8, Joined, INVALID, output, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 413 /* --out */, anonymous_7, Separate, grp_general, output, nullptr, 0, DefaultVis, 0,
       "Alias for -o", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<filename>", nullptr)
OPTION(3, 419 /* -o */, output, Separate, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Specify the output file. Defaults to <input file>.dwarf", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<filename>", nullptr)
OPTION(6, 422 /* --quiet */, quiet, Flag, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Enable quiet mode.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 430 /* -q */, anonymous_2, Flag, grp_general, quiet, nullptr, 0, DefaultVis, 0,
       "Alias for --quiet", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 433 /* --remarks-drop-without-debug */, remarks_drop_without_debug, Flag, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Drop remarks without valid debug locations. Without this flags, all remarks are kept.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 462 /* --remarks-output-format= */, anonymous_18, Joined, INVALID, remarks_output_format, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 487 /* --remarks-output-format */, remarks_output_format, Separate, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Specify the format to be used when serializing the linked remarks.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<format>", nullptr)
OPTION(6, 511 /* --remarks-prepend-path= */, anonymous_17, Joined, INVALID, remarks_prepend_path, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 535 /* --remarks-prepend-path */, remarks_prepend_path, Separate, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Specify a directory to prepend to the paths of the external remark files.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<path>", nullptr)
OPTION(6, 558 /* --reproducer= */, anonymous_15, Joined, INVALID, reproducer, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 572 /* --reproducer */, reproducer, Separate, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Specify the reproducer generation mode. Valid options are 'GenerateOnExit', 'GenerateOnCrash', 'Off'.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<mode>", nullptr)
OPTION(6, 585 /* --statistics */, statistics, Flag, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Print statistics about the contribution of each object file to the linked debug info. This prints a table after linking with the object file name, the size of the debug info in the object file (in bytes) and the size contributed (in bytes) to the linked dSYM. The table is sorted by the output size listing the object files with the largest contribution first.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 598 /* --symtab */, symtab, Flag, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Dumps the symbol table found in executable or object file(s) and exits.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 607 /* -S */, assembly, Flag, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Output textual assembly instead of a binary dSYM companion file.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 610 /* -s */, anonymous_4, Flag, grp_general, symtab, nullptr, 0, DefaultVis, 0,
       "Alias for --symtab", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 613 /* --toolchain */, toolchain, Separate, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Embed toolchain information in dSYM bundle.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<toolchain>", nullptr)
OPTION(6, 625 /* --update */, update, Flag, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Updates existing dSYM files to contain the latest accelerator tables and other DWARF optimizations.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 634 /* --use-reproducer= */, anonymous_16, Joined, INVALID, use_reproducer, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 652 /* --use-reproducer */, use_reproducer, Separate, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Use the object files from the given reproducer path.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<path>", nullptr)
OPTION(1, 669 /* -u */, anonymous_6, Flag, grp_general, update, nullptr, 0, DefaultVis, 0,
       "Alias for --update", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 672 /* --verbose */, verbose, Flag, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Enable verbose mode.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 682 /* --verify-dwarf= */, anonymous_3, Joined, INVALID, verify_dwarf, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 698 /* --verify-dwarf */, verify_dwarf, Separate, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Run the DWARF verifier on the input and/or output. Valid options are 'none, 'input', 'output', 'all' or 'auto' which runs the output verifier only if input verification passed.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<verification mode>", nullptr)
OPTION(6, 713 /* --verify */, verify, Flag, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Alias for --verify-dwarf=output", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(6, 722 /* --version */, version, Flag, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Prints the dsymutil version.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 732 /* -v */, anonymous_1, Flag, grp_general, version, nullptr, 0, DefaultVis, 0,
       "Alias for --version", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 735 /* -y */, yaml_input, Flag, grp_general, INVALID, nullptr, 0, DefaultVis, 0,
       "Treat the input file is a YAML debug map rather than a binary.", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
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


