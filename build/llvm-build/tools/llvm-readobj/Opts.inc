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
  "--addrsig\0"
  "--all\0"
  "--arch-specific\0"
  "--auxiliary-header\0"
  "-A\0"
  "-a\0"
  "--bb-addr-map\0"
  "--cg-profile\0"
  "--codeview-ghash\0"
  "--codeview-merged-types\0"
  "--codeview-subsection-bytes\0"
  "--codeview\0"
  "--coff-basereloc\0"
  "--coff-debug-directory\0"
  "--coff-directives\0"
  "--coff-exports\0"
  "--coff-imports\0"
  "--coff-load-config\0"
  "--coff-resources\0"
  "--coff-tls-directory\0"
  "-C\0"
  "--decompress\0"
  "--demangle\0"
  "--dependent-libraries\0"
  "--dt\0"
  "--dyn-relocations\0"
  "--dyn-symbols\0"
  "--dyn-syms\0"
  "--dynamic-table\0"
  "--dynamic\0"
  "-d\0"
  "--elf-cg-profile\0"
  "--elf-hash-histogram\0"
  "--elf-linker-options\0"
  "--elf-output-style=\0"
  "--elf-output-style\0"
  "--elf-section-groups\0"
  "--exception-section\0"
  "--expand-relocs\0"
  "--extra-sym-info\0"
  "-e\0"
  "--file-headers\0"
  "--file-header\0"
  "--gnu-hash-table\0"
  "-g\0"
  "--hash-symbols\0"
  "--hash-table\0"
  "--headers\0"
  "--help\0"
  "--hex-dump=\0"
  "--hex-dump\0"
  "--histogram\0"
  "-h\0"
  "-I\0"
  "--loader-section-header\0"
  "--loader-section-relocations\0"
  "--loader-section-symbols\0"
  "-l\0"
  "--macho-data-in-code\0"
  "--macho-dysymtab\0"
  "--macho-indirect-symbols\0"
  "--macho-linker-options\0"
  "--macho-segment\0"
  "--macho-version-min\0"
  "--memtag\0"
  "--needed-libs\0"
  "--no-demangle\0"
  "--notes\0"
  "-n\0"
  "--pretty-pgo-analysis-map\0"
  "--pretty-print\0"
  "--program-headers\0"
  "-p\0"
  "--relocations\0"
  "--relocs\0"
  "-r\0"
  "--sd\0"
  "--section-data\0"
  "--section-details\0"
  "--section-groups\0"
  "--section-headers\0"
  "--section-mapping=false\0"
  "--section-mapping\0"
  "--section-relocations\0"
  "--section-symbols\0"
  "--sections\0"
  "--segments\0"
  "--sort-symbols=\0"
  "--sort-symbols\0"
  "--sr\0"
  "--stack-sizes\0"
  "--stackmap\0"
  "--string-dump=\0"
  "--string-dump\0"
  "--string-table\0"
  "--st\0"
  "--symbols\0"
  "--syms\0"
  "-S\0"
  "-s\0"
  "-t\0"
  "--unwind\0"
  "-u\0"
  "--version-info\0"
  "--version\0"
  "-V\0"
  "--wide\0"
  "-W\0"
  "-X\0"
  "-x\0"
  "-z\0"
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
OPTION(0 /* no prefixes */, 6 /* kind */, grp_coff, Group, INVALID, INVALID, nullptr, 0, 0, 0,
       "OPTIONS (PE/COFF specific)", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(0 /* no prefixes */, 6 /* kind */, grp_elf, Group, INVALID, INVALID, nullptr, 0, 0, 0,
       "OPTIONS (ELF specific)", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(0 /* no prefixes */, 6 /* kind */, grp_mach_o, Group, INVALID, INVALID, nullptr, 0, 0, 0,
       "OPTIONS (Mach-O specific)", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(0 /* no prefixes */, 6 /* kind */, grp_xcoff, Group, INVALID, INVALID, nullptr, 0, 0, 0,
       "OPTIONS (XCOFF specific)", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)

//////////
// Options

OPTION(0, 11 /* <input> */, INPUT, Input, INVALID, INVALID, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(0, 19 /* <unknown> */, UNKNOWN, Unknown, INVALID, INVALID, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 29 /* --addrsig */, addrsig, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display address-significance table", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 39 /* --all */, all, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Equivalent to setting: --file-header, --program-headers, --section-headers, --symbols, --relocations, --dynamic-table, --notes, --version-info, --unwind, --section-groups and --histogram", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 45 /* --arch-specific */, arch_specific, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display architecture-specific information", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 61 /* --auxiliary-header */, auxiliary_header, Flag, grp_xcoff, INVALID, nullptr, 0, DefaultVis, 0,
       "Display the auxiliary header", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 80 /* -A */, anonymous_19, Flag, INVALID, arch_specific, nullptr, 0, DefaultVis, 0,
       "Alias for --arch-specific", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 83 /* -a */, anonymous_20, Flag, INVALID, all, nullptr, 0, DefaultVis, 0,
       "Alias for --all", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 86 /* --bb-addr-map */, bb_addr_map, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display the BB address map section", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 100 /* --cg-profile */, cg_profile, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display call graph profile section", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 113 /* --codeview-ghash */, codeview_ghash, Flag, grp_coff, INVALID, nullptr, 0, DefaultVis, 0,
       "Enable global hashing for CodeView type stream de-duplication", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 130 /* --codeview-merged-types */, codeview_merged_types, Flag, grp_coff, INVALID, nullptr, 0, DefaultVis, 0,
       "Display the merged CodeView type stream", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 154 /* --codeview-subsection-bytes */, codeview_subsection_bytes, Flag, grp_coff, INVALID, nullptr, 0, DefaultVis, 0,
       "Dump raw contents of codeview debug sections and records", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 182 /* --codeview */, codeview, Flag, grp_coff, INVALID, nullptr, 0, DefaultVis, 0,
       "Display CodeView debug information", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 193 /* --coff-basereloc */, coff_basereloc, Flag, grp_coff, INVALID, nullptr, 0, DefaultVis, 0,
       "Display .reloc section", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 210 /* --coff-debug-directory */, coff_debug_directory, Flag, grp_coff, INVALID, nullptr, 0, DefaultVis, 0,
       "Display debug directory", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 233 /* --coff-directives */, coff_directives, Flag, grp_coff, INVALID, nullptr, 0, DefaultVis, 0,
       "Display .drectve section", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 251 /* --coff-exports */, coff_exports, Flag, grp_coff, INVALID, nullptr, 0, DefaultVis, 0,
       "Display export table", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 266 /* --coff-imports */, coff_imports, Flag, grp_coff, INVALID, nullptr, 0, DefaultVis, 0,
       "Display import table", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 281 /* --coff-load-config */, coff_load_config, Flag, grp_coff, INVALID, nullptr, 0, DefaultVis, 0,
       "Display load config", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 300 /* --coff-resources */, coff_resources, Flag, grp_coff, INVALID, nullptr, 0, DefaultVis, 0,
       "Display .rsrc section", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 317 /* --coff-tls-directory */, coff_tls_directory, Flag, grp_coff, INVALID, nullptr, 0, DefaultVis, 0,
       "Display TLS directory", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 338 /* -C */, anonymous_21, Flag, INVALID, demangle, nullptr, 0, DefaultVis, 0,
       "Alias for --demangle", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 341 /* --decompress */, decompress, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Dump decompressed section content when used with -x or -p", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 354 /* --demangle */, demangle, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Demangle symbol names", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 365 /* --dependent-libraries */, dependent_libraries, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display the dependent libraries section", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 387 /* --dt */, anonymous_5, Flag, INVALID, dyn_syms, nullptr, 0, DefaultVis, 0,
       "Alias for --dyn-syms", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 392 /* --dyn-relocations */, dyn_relocations, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display the dynamic relocation entries in the file", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 410 /* --dyn-symbols */, anonymous_9, Flag, INVALID, dyn_syms, nullptr, 0, DefaultVis, 0,
       "Alias for --dyn-syms", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 424 /* --dyn-syms */, dyn_syms, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display the dynamic symbol table", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 435 /* --dynamic-table */, dynamic_table, Flag, grp_elf, INVALID, nullptr, 0, DefaultVis, 0,
       "Display the dynamic section table", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 451 /* --dynamic */, anonymous_10, Flag, INVALID, dynamic_table, nullptr, 0, DefaultVis, 0,
       "Alias for --dynamic-table", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 461 /* -d */, anonymous_22, Flag, grp_elf, dynamic_table, nullptr, 0, DefaultVis, 0,
       "Alias for --dynamic-table", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 464 /* --elf-cg-profile */, anonymous_11, Flag, INVALID, cg_profile, nullptr, HelpHidden, DefaultVis, 0,
       "Alias for --cg-profile", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 481 /* --elf-hash-histogram */, anonymous_12, Flag, INVALID, histogram, nullptr, HelpHidden, DefaultVis, 0,
       "Alias for --histogram", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 502 /* --elf-linker-options */, elf_linker_options, Flag, grp_elf, INVALID, nullptr, 0, DefaultVis, 0,
       "Display the .linker-options section", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 523 /* --elf-output-style= */, elf_output_style_EQ, Joined, grp_elf, INVALID, nullptr, 0, DefaultVis, 0,
       "Specify ELF dump style: LLVM, GNU, JSON", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 543 /* --elf-output-style */, anonymous_3, Separate, grp_elf, elf_output_style_EQ, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 562 /* --elf-section-groups */, anonymous_13, Flag, INVALID, section_groups, nullptr, HelpHidden, DefaultVis, 0,
       "Alias for --section-groups", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 583 /* --exception-section */, exception_section, Flag, grp_xcoff, INVALID, nullptr, 0, DefaultVis, 0,
       "Display the exception section entries", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 603 /* --expand-relocs */, expand_relocs, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Expand each shown relocation to multiple lines", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 619 /* --extra-sym-info */, extra_sym_info, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display extra information when showing symbols", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 636 /* -e */, anonymous_23, Flag, INVALID, headers, nullptr, 0, DefaultVis, 0,
       "Alias for --headers", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 639 /* --file-headers */, anonymous_14, Flag, INVALID, file_header, nullptr, HelpHidden, DefaultVis, 0,
       "Alias for --file-header", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 654 /* --file-header */, file_header, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display file header", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 668 /* --gnu-hash-table */, gnu_hash_table, Flag, grp_elf, INVALID, nullptr, 0, DefaultVis, 0,
       "Display the GNU hash table for dynamic symbols", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 685 /* -g */, anonymous_24, Flag, grp_elf, section_groups, nullptr, 0, DefaultVis, 0,
       "Alias for --section-groups", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 688 /* --hash-symbols */, hash_symbols, Flag, grp_elf, INVALID, nullptr, 0, DefaultVis, 0,
       "Display the dynamic symbols derived from the hash section", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 703 /* --hash-table */, hash_table, Flag, grp_elf, INVALID, nullptr, 0, DefaultVis, 0,
       "Display .hash section", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 716 /* --headers */, headers, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Equivalent to setting: --file-header, --program-headers, --section-headers", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 726 /* --help */, help, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display this help", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 733 /* --hex-dump= */, hex_dump_EQ, Joined, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display the specified section(s) as hexadecimal bytes", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<name or index>", nullptr)
OPTION(3, 745 /* --hex-dump */, anonymous_0, Separate, INVALID, hex_dump_EQ, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<name or index>", nullptr)
OPTION(3, 756 /* --histogram */, histogram, Flag, grp_elf, INVALID, nullptr, 0, DefaultVis, 0,
       "Display bucket list histogram for hash sections", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 768 /* -h */, anonymous_25, Flag, INVALID, file_header, nullptr, 0, DefaultVis, 0,
       "Alias for --file-header", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 771 /* -I */, anonymous_26, Flag, grp_elf, histogram, nullptr, 0, DefaultVis, 0,
       "Alias for --histogram", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 774 /* --loader-section-header */, loader_section_header, Flag, grp_xcoff, INVALID, nullptr, 0, DefaultVis, 0,
       "Display the loader section header", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 798 /* --loader-section-relocations */, loader_section_relocations, Flag, grp_xcoff, INVALID, nullptr, 0, DefaultVis, 0,
       "Display the loader section relocation entries", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 827 /* --loader-section-symbols */, loader_section_symbols, Flag, grp_xcoff, INVALID, nullptr, 0, DefaultVis, 0,
       "Display the loader section symbol table", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 852 /* -l */, anonymous_27, Flag, INVALID, program_headers, nullptr, 0, DefaultVis, 0,
       "Alias for --program-headers", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 855 /* --macho-data-in-code */, macho_data_in_code, Flag, grp_mach_o, INVALID, nullptr, 0, DefaultVis, 0,
       "Display Data in Code command", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 876 /* --macho-dysymtab */, macho_dysymtab, Flag, grp_mach_o, INVALID, nullptr, 0, DefaultVis, 0,
       "Display Dysymtab command", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 893 /* --macho-indirect-symbols */, macho_indirect_symbols, Flag, grp_mach_o, INVALID, nullptr, 0, DefaultVis, 0,
       "Display indirect symbols", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 918 /* --macho-linker-options */, macho_linker_options, Flag, grp_mach_o, INVALID, nullptr, 0, DefaultVis, 0,
       "Display linker options", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 941 /* --macho-segment */, macho_segment, Flag, grp_mach_o, INVALID, nullptr, 0, DefaultVis, 0,
       "Display Segment command", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 957 /* --macho-version-min */, macho_version_min, Flag, grp_mach_o, INVALID, nullptr, 0, DefaultVis, 0,
       "Display version min command", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 977 /* --memtag */, memtag, Flag, grp_elf, INVALID, nullptr, 0, DefaultVis, 0,
       "Display memory tagging metadata (modes, Android notes, global descriptors)", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 986 /* --needed-libs */, needed_libs, Flag, grp_elf, INVALID, nullptr, 0, DefaultVis, 0,
       "Display the needed libraries", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1000 /* --no-demangle */, no_demangle, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Do not demangle symbol names (default)", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1014 /* --notes */, notes, Flag, grp_elf, INVALID, nullptr, 0, DefaultVis, 0,
       "Display notes", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 1022 /* -n */, anonymous_28, Flag, INVALID, notes, nullptr, 0, DefaultVis, 0,
       "Alias for --notes", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1025 /* --pretty-pgo-analysis-map */, pretty_pgo_analysis_map, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display PGO analysis values with formatting rather than raw numbers", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1051 /* --pretty-print */, pretty_print, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Pretty print JSON output", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1066 /* --program-headers */, program_headers, Flag, grp_elf, INVALID, nullptr, 0, DefaultVis, 0,
       "Display program headers", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 1084 /* -p */, anonymous_29, JoinedOrSeparate, INVALID, string_dump_EQ, nullptr, 0, DefaultVis, 0,
       "Alias for --string-dump", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<name or index>", nullptr)
OPTION(3, 1087 /* --relocations */, anonymous_15, Flag, INVALID, relocs, nullptr, 0, DefaultVis, 0,
       "Alias for --relocs", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1101 /* --relocs */, relocs, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display the relocation entries in the file", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 1110 /* -r */, anonymous_30, Flag, INVALID, relocs, nullptr, 0, DefaultVis, 0,
       "Alias for --relocs", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1113 /* --sd */, anonymous_6, Flag, INVALID, section_data, nullptr, 0, DefaultVis, 0,
       "Alias for --section-data", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1118 /* --section-data */, section_data, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display section data for each section shown. This option has no effect for GNU style output", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1133 /* --section-details */, section_details, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display the section details", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1151 /* --section-groups */, section_groups, Flag, grp_elf, INVALID, nullptr, 0, DefaultVis, 0,
       "Display section groups", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1168 /* --section-headers */, section_headers, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display section headers", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1186 /* --section-mapping=false */, section_mapping_EQ_false, Flag, INVALID, INVALID, nullptr, HelpHidden, DefaultVis, 0,
       "Don't display the section to segment mapping", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1210 /* --section-mapping */, section_mapping, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display the section to segment mapping", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1228 /* --section-relocations */, section_relocations, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display relocations for each section shown. This option has no effect for GNU style output", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1250 /* --section-symbols */, section_symbols, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display symbols for each section shown. This option has no effect for GNU style output", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1268 /* --sections */, anonymous_16, Flag, INVALID, section_headers, nullptr, 0, DefaultVis, 0,
       "Alias for --section-headers", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1279 /* --segments */, anonymous_17, Flag, grp_elf, program_headers, nullptr, 0, DefaultVis, 0,
       "Alias for --program-headers", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1290 /* --sort-symbols= */, sort_symbols_EQ, Joined, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Specify the keys to sort the symbols before displaying symtab", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1306 /* --sort-symbols */, anonymous_1, Separate, INVALID, sort_symbols_EQ, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1321 /* --sr */, anonymous_8, Flag, INVALID, section_relocations, nullptr, 0, DefaultVis, 0,
       "Alias for --section-relocations", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1326 /* --stack-sizes */, stack_sizes, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display contents of all stack sizes sections. This option has no effect for GNU style output", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1340 /* --stackmap */, stackmap, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display contents of stackmap section", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1351 /* --string-dump= */, string_dump_EQ, Joined, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display the specified section(s) as a list of strings", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<name or index>", nullptr)
OPTION(3, 1366 /* --string-dump */, anonymous_2, Separate, INVALID, string_dump_EQ, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<name or index>", nullptr)
OPTION(3, 1380 /* --string-table */, string_table, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display the string table (only for XCOFF now)", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1395 /* --st */, anonymous_7, Flag, INVALID, section_symbols, nullptr, 0, DefaultVis, 0,
       "Alias for --section-symbols", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1400 /* --symbols */, symbols, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display the symbol table. Also display the dynamic symbol table when using GNU output style for ELF", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1410 /* --syms */, anonymous_18, Flag, INVALID, symbols, nullptr, 0, DefaultVis, 0,
       "Alias for --symbols", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 1417 /* -S */, anonymous_31, Flag, INVALID, section_headers, nullptr, 0, DefaultVis, 0,
       "Alias for --section-headers", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 1420 /* -s */, anonymous_32, Flag, INVALID, symbols, nullptr, 0, DefaultVis, 0,
       "Alias for --symbols", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 1423 /* -t */, anonymous_33, Flag, INVALID, section_details, nullptr, 0, DefaultVis, 0,
       "Alias for --section-details", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1426 /* --unwind */, unwind, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display unwind information", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 1435 /* -u */, anonymous_34, Flag, INVALID, unwind, nullptr, 0, DefaultVis, 0,
       "Alias for --unwind", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1438 /* --version-info */, version_info, Flag, grp_elf, INVALID, nullptr, 0, DefaultVis, 0,
       "Display version sections", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1453 /* --version */, version, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Display the version", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 1463 /* -V */, anonymous_36, Flag, grp_elf, version_info, nullptr, 0, DefaultVis, 0,
       "Alias for --version-info", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(3, 1466 /* --wide */, wide, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Ignored for GNU readelf compatibility", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 1473 /* -W */, anonymous_4, Flag, INVALID, wide, nullptr, 0, DefaultVis, 0,
       "Ignored for GNU readelf compatibility", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 1476 /* -X */, anonymous_35, Flag, grp_elf, extra_sym_info, nullptr, 0, DefaultVis, 0,
       "Alias for --extra-sym-info", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 1479 /* -x */, anonymous_37, JoinedOrSeparate, INVALID, hex_dump_EQ, nullptr, 0, DefaultVis, 0,
       "Alias for --hex-dump", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "<name or index>", nullptr)
OPTION(1, 1482 /* -z */, anonymous_38, Flag, INVALID, decompress, nullptr, 0, DefaultVis, 0,
       "Alias for --decompress", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
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


