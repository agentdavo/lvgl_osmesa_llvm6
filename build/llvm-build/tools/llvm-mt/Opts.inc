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
  "/\0"
  "unsupported\0"
  "<input>\0"
  "<unknown>\0"
  "/?\0"
  "/canonicalize:\0"
  "/category\0"
  "/check_for_duplicates:\0"
  "/dll:\0"
  "/hashupdate:\0"
  "/hashupdate\0"
  "/help\0"
  "/h\0"
  "/identity:\0"
  "/inputresource:\0"
  "/makecdfs:\0"
  "/managedassemblyname:\0"
  "/manifest\0"
  "/nodependency\0"
  "/nologo\0"
  "/notify_update\0"
  "/out:\0"
  "/outputresource:\0"
  "/outputresource\0"
  "/replacements:\0"
  "/rgs:\0"
  "/tlb:\0"
  "/updateresource:\0"
  "/validate_file_hashes:\0"
  "/validate_manifest\0"
  "/verbose\0"
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
  2 /* prefixes */, 3 /* '/' */, 1 /* '-' */
};
#endif // OPTTABLE_PREFIXES_TABLE_CODE

/////////
// Prefix Union

#ifdef OPTTABLE_PREFIXES_UNION_CODE
static constexpr llvm::StringTable::Offset OptionPrefixesUnion[] = {
  1 /* '-' */,   3 /* '/' */
};
#endif // OPTTABLE_PREFIXES_UNION_CODE

/////////
// ValuesCode

#ifdef OPTTABLE_VALUES_CODE
#endif
/////////
// Groups

#ifdef OPTION
OPTION(0 /* no prefixes */, 5 /* unsupported */, unsupported, Group, INVALID, INVALID, nullptr, 0, 0, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)

//////////
// Options

OPTION(0, 17 /* <input> */, INPUT, Input, INVALID, INVALID, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(0, 25 /* <unknown> */, UNKNOWN, Unknown, INVALID, INVALID, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 35 /* /? */, help, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 38 /* /canonicalize: */, canonicalize, Flag, unsupported, INVALID, nullptr, 0, DefaultVis, 0,
       "Not supported", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 53 /* /category */, category, Flag, unsupported, INVALID, nullptr, 0, DefaultVis, 0,
       "Not supported", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 63 /* /check_for_duplicates: */, check_for_duplicates, Flag, unsupported, INVALID, nullptr, 0, DefaultVis, 0,
       "Not supported", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 86 /* /dll: */, dll, Joined, unsupported, INVALID, nullptr, 0, DefaultVis, 0,
       "Not supported", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "dll", nullptr)
OPTION(1, 92 /* /hashupdate: */, hash_update, Joined, unsupported, INVALID, nullptr, 0, DefaultVis, 0,
       "Not supported", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "file", nullptr)
OPTION(1, 105 /* /hashupdate */, hash_update_flag, Flag, unsupported, hash_update, nullptr, 0, DefaultVis, 0,
       "Not supported", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 117 /* /help */, help_long, Flag, INVALID, help, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 123 /* /h */, h, Flag, INVALID, help, nullptr, 0, DefaultVis, 0, nullptr, (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 126 /* /identity: */, identity, Joined, unsupported, INVALID, nullptr, 0, DefaultVis, 0,
       "Not supported", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "identity", nullptr)
OPTION(1, 137 /* /inputresource: */, input_resource, Joined, unsupported, INVALID, nullptr, 0, DefaultVis, 0,
       "Not supported", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "file", nullptr)
OPTION(1, 153 /* /makecdfs: */, make_cdfs, Flag, unsupported, INVALID, nullptr, 0, DefaultVis, 0,
       "Not supported", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 164 /* /managedassemblyname: */, managed_assembly_name, Joined, unsupported, INVALID, nullptr, 0, DefaultVis, 0,
       "Not supported", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "assembly", nullptr)
OPTION(1, 186 /* /manifest */, manifest, Separate, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Used to specify each manifest that need to be processed", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "manifest", nullptr)
OPTION(1, 196 /* /nodependency */, no_dependency, Flag, unsupported, INVALID, nullptr, 0, DefaultVis, 0,
       "Not supported", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 210 /* /nologo */, no_logo, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "No effect as this tool never writes copyright data.  Included for parity", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 218 /* /notify_update */, notify_update, Flag, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Exit with a special exit code if the output file has changed", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 233 /* /out: */, out, Joined, INVALID, INVALID, nullptr, 0, DefaultVis, 0,
       "Name of the output manifest.  If this is skipped and only one manifest is being operated upon by the tool, that manifest is modified in place", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "manifest", nullptr)
OPTION(1, 239 /* /outputresource: */, output_resource, Joined, unsupported, INVALID, nullptr, 0, DefaultVis, 0,
       "Not supported", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "file", nullptr)
OPTION(1, 256 /* /outputresource */, output_resource_flag, Flag, unsupported, output_resource, nullptr, 0, DefaultVis, 0,
       "Not supported", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 272 /* /replacements: */, replacements, Joined, unsupported, INVALID, nullptr, 0, DefaultVis, 0,
       "Not supported", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "file", nullptr)
OPTION(1, 287 /* /rgs: */, rgs, Joined, unsupported, INVALID, nullptr, 0, DefaultVis, 0,
       "Not supported", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "script", nullptr)
OPTION(1, 293 /* /tlb: */, tlb, Joined, unsupported, INVALID, nullptr, 0, DefaultVis, 0,
       "Not supported", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "file", nullptr)
OPTION(1, 299 /* /updateresource: */, update_resource, Joined, unsupported, INVALID, nullptr, 0, DefaultVis, 0,
       "Not supported", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "file", nullptr)
OPTION(1, 316 /* /validate_file_hashes: */, validate_file_hashes, Joined, unsupported, INVALID, nullptr, 0, DefaultVis, 0,
       "Not supported", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), "", nullptr)
OPTION(1, 339 /* /validate_manifest */, validate_manifest, Flag, unsupported, INVALID, nullptr, 0, DefaultVis, 0,
       "Not supported", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
OPTION(1, 358 /* /verbose */, verbose, Flag, unsupported, INVALID, nullptr, 0, DefaultVis, 0,
       "Not supported", (std::array<std::pair<std::array<unsigned, 2>, const char*>, 1>{{ {std::array<unsigned, 2>{{0, 0}}, nullptr} }}), nullptr, nullptr)
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


