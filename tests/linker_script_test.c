//
//  linker_script_test.c
//  c_compiler
//
//  Unit tests for GNU ld / LLVM lld linker-script parsing.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "elf.h"
#include "linker_config.h"
#include "linker_script.h"

static int g_failures = 0;

#define CHECK(cond)                                                     \
  do {                                                                  \
    if (!(cond)) {                                                      \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);   \
      g_failures++;                                                     \
    }                                                                   \
  } while (0)

static ConfigSegment* FindSeg(LinkerConfig* config, ConfigSegmentType type) {
  return LinkerConfigFindSegment(config, type);
}

static ConfigRegion* FindRegion(ConfigSegment* segment, const char* name) {
  if (segment == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < segment->regions.length; i++) {
    ConfigRegion* region = segment->regions.value.p[i];
    if (StringEqual(&region->name, name)) {
      return region;
    }
  }
  return NULL;
}

static bool RegionHas(const ConfigRegion* region, const char* section) {
  if (region == NULL) {
    return false;
  }
  return LinkerConfigSectionMatches(&region->sections, section);
}

static bool HasDiscard(const LinkerConfig* config, const char* section) {
  return LinkerConfigShouldDiscard(config, section);
}

static ConfigScriptSymbol* FindSym(LinkerConfig* config, const char* name) {
  for (size_t i = 0; i < config->script_symbols.length; i++) {
    ConfigScriptSymbol* sym = config->script_symbols.value.p[i];
    if (StringEqual(&sym->name, name)) {
      return sym;
    }
  }
  return NULL;
}

static void TestStandardScript(void) {
  static const char kScript[] =
      "/* GNU ld compatible script */\n"
      "ENTRY(_start)\n"
      "OUTPUT_ARCH(riscv)\n"
      "PHDRS\n"
      "{\n"
      "  text PT_LOAD FLAGS(5);\n"
      "  data PT_LOAD FLAGS(6);\n"
      "  dynamic PT_DYNAMIC;\n"
      "  interp PT_INTERP;\n"
      "}\n"
      "MEMORY\n"
      "{\n"
      "  text (rx) : ORIGIN = 0x400000000, LENGTH = 0\n"
      "  data (rw) : org = 0x410000000, len = 64K\n"
      "}\n"
      "SECTIONS\n"
      "{\n"
      "  .text : { *(.text .rodata) } > text :text\n"
      "  .data : { KEEP(*(.data)) *(.got) } > data :data\n"
      "  .bss : { *(.bss) } > data\n"
      "}\n";

  LinkerConfig config = {0};
  CHECK(LinkerScriptParseString("test.ld", kScript, ELF_MACHINE_TYPE_RISC_V,
                                &config));
  CHECK(config.errors == 0);
  CHECK(StringEqual(&config.entry_symbol, "_start"));

  ConfigSegment* text = FindSeg(&config, kConfigSegmentTypeText);
  ConfigSegment* data = FindSeg(&config, kConfigSegmentTypeData);
  CHECK(text != NULL);
  CHECK(data != NULL);
  CHECK(FindSeg(&config, kConfigSegmentTypeDynamic) != NULL);
  CHECK(FindSeg(&config, kConfigSegmentTypeInterp) != NULL);
  CHECK(text->alignment == 0x1000);
  CHECK(data->alignment == 0x1000);

  ConfigRegion* text_r = FindRegion(text, "text");
  ConfigRegion* data_r = FindRegion(data, "data");
  CHECK(text_r != NULL);
  CHECK(data_r != NULL);
  CHECK(text_r->start_addr == 0x400000000ull);
  CHECK(text_r->falign);
  CHECK(data_r->start_addr == 0x410000000ull);
  CHECK(data_r->size == 64ull * 1024ull);
  CHECK(RegionHas(text_r, ".text"));
  CHECK(RegionHas(text_r, ".rodata"));
  CHECK(RegionHas(data_r, ".data"));
  CHECK(RegionHas(data_r, ".got"));
  CHECK(RegionHas(data_r, ".bss"));

  LinkerConfigDestruct(&config);
}

static void Test6502RomBuiltin(void) {
  LinkerConfig config = {0};
  CHECK(LinkerScriptLoadBuiltin(ELF_MACHINE_TYPEW65C02, true, "rom",
                                &config));
  ConfigSegment* text = FindSeg(&config, kConfigSegmentTypeText);
  CHECK(text != NULL);
  CHECK(text->alignment == 1);
  ConfigRegion* rom = FindRegion(text, "text");
  ConfigRegion* boot = FindRegion(text, "boot");
  ConfigRegion* vec = FindRegion(text, "hwvectors");
  CHECK(rom != NULL && boot != NULL && vec != NULL);
  CHECK(rom->start_addr == 0xc000);
  CHECK(rom->size == 0x3f00);
  CHECK(!rom->falign);
  CHECK(boot->start_addr == 0xff00);
  CHECK(vec->start_addr == 0xfffa);
  CHECK(RegionHas(rom, ".text"));
  CHECK(RegionHas(rom, ".rodata"));
  CHECK(RegionHas(rom, ".davecc_stacktrace"));
  CHECK(RegionHas(boot, ".boot"));
  CHECK(RegionHas(vec, ".hwvectors"));
  LinkerConfigDestruct(&config);
}

static void TestIntromScript(void) {
  static const char kScript[] =
      "MEMORY\n"
      "{\n"
      "  text (rx) : ORIGIN = 0xc000, LENGTH = 0x3ffa\n"
      "  hwvectors (r) : ORIGIN = 0xfffa, LENGTH = 6\n"
      "}\n"
      "SECTIONS\n"
      "{\n"
      "  .text : { *(.text) *(.rodata) } > text\n"
      "  .hwvectors : { *(.hwvectors) } > hwvectors\n"
      "}\n";
  LinkerConfig config = {0};
  CHECK(LinkerScriptParseString("introm.map", kScript, ELF_MACHINE_TYPEW65C02,
                                &config));
  ConfigSegment* text = FindSeg(&config, kConfigSegmentTypeText);
  ConfigRegion* rom = FindRegion(text, "text");
  ConfigRegion* vec = FindRegion(text, "hwvectors");
  CHECK(rom != NULL && vec != NULL);
  CHECK(rom->start_addr == 0xc000);
  CHECK(rom->size == 0x3ffa);
  CHECK(vec->start_addr == 0xfffa);
  CHECK(vec->size == 6);
  CHECK(RegionHas(rom, ".text"));
  CHECK(RegionHas(rom, ".rodata"));
  CHECK(RegionHas(vec, ".hwvectors"));
  CHECK(!RegionHas(rom, ".davecc_stacktrace"));
  LinkerConfigDestruct(&config);
}

static void TestElfBuiltins(void) {
  static const struct {
    int machine;
    bool is_64_bit;
    uint64_t text_start;
  } builtins[] = {
      {ELF_MACHINE_TYPE_RISC_V, false, 0x40000000ull},
      {ELF_MACHINE_TYPE_RISC_V, true, 0x400000000ull},
      {ELF_MACHINE_TYPE_PCODE, true, 0x400000000ull},
      {ELF_MACHINE_TYPE_BPF, true, 0x400000000ull},
      {ELF_MACHINE_TYPE_AARCH64, true, 0x400000000ull},
      {ELF_MACHINE_TYPE_ARM, false, 0x40000000ull},
      {ELF_MACHINE_TYPE_X86, false, 0x08048000ull},
      {ELF_MACHINE_TYPE_X86_64, true, 0x400000000ull},
  };
  for (size_t i = 0; i < sizeof(builtins) / sizeof(builtins[0]); i++) {
    LinkerConfig config = {0};
    CHECK(LinkerScriptLoadBuiltin(builtins[i].machine, builtins[i].is_64_bit,
                                  "program", &config));
    ConfigSegment* text = FindSeg(&config, kConfigSegmentTypeText);
    ConfigSegment* data = FindSeg(&config, kConfigSegmentTypeData);
    CHECK(text != NULL && data != NULL);
    ConfigRegion* text_r = FindRegion(text, "text");
    ConfigRegion* data_r = FindRegion(data, "data");
    CHECK(text_r != NULL && data_r != NULL);
    CHECK(text->alignment == 0x1000);
    CHECK(text_r->falign);
    CHECK(RegionHas(text_r, ".text"));
    CHECK(RegionHas(data_r, ".data"));
    CHECK(RegionHas(data_r, ".bss"));
    CHECK(text_r->start_addr == builtins[i].text_start);
    if (builtins[i].machine == ELF_MACHINE_TYPE_ARM) {
      CHECK(RegionHas(text_r, ".ARM.exidx"));
      CHECK(RegionHas(text_r, ".ARM.extab"));
    }
    if (builtins[i].machine == ELF_MACHINE_TYPE_PCODE) {
      CHECK(RegionHas(text_r, ".davecc_except_table"));
    }
    LinkerConfigDestruct(&config);
  }
}

static void TestCommentsAndHash(void) {
  static const char kScript[] =
      "# hash comment\n"
      "// line comment\n"
      "MEMORY { text (rx) : ORIGIN = 0x800, LENGTH = 0x100 /* block */ }\n"
      "SECTIONS { .text : { *(.text) } > text }\n";
  LinkerConfig config = {0};
  CHECK(LinkerScriptParseString("comments.ld", kScript, ELF_MACHINE_TYPEW65C02,
                                &config));
  ConfigRegion* text =
      FindRegion(FindSeg(&config, kConfigSegmentTypeText), "text");
  CHECK(text != NULL);
  CHECK(text->start_addr == 0x800);
  CHECK(text->size == 0x100);
  LinkerConfigDestruct(&config);
}

static void TestMissingBuiltin(void) {
  LinkerConfig config = {0};
  CHECK(!LinkerScriptLoadBuiltin(ELF_MACHINE_TYPEW65C02, false,
                                 "no-such-layout", &config));
  CHECK(config.errors != 0);
  LinkerConfigDestruct(&config);
}

static void TestWildcardsDiscardProvide(void) {
  static const char kScript[] =
      "MEMORY { rom (rx) : ORIGIN = 0x1000, LENGTH = 8K }\n"
      "SECTIONS {\n"
      "  .text : {\n"
      "    KEEP(*(.text*))\n"
      "    SORT(*(.rodata*))\n"
      "    . = ALIGN(8);\n"
      "    _etext = .;\n"
      "  } > rom\n"
      "  .bss : { *(.bss* COMMON) } > rom\n"
      "  /DISCARD/ : { *(.comment .note*) }\n"
      "  PROVIDE(_end = .);\n"
      "}\n";
  LinkerConfig config = {0};
  CHECK(LinkerScriptParseString("typical.ld", kScript, ELF_MACHINE_TYPE_RISC_V,
                                &config));
  ConfigRegion* rom =
      FindRegion(FindSeg(&config, kConfigSegmentTypeText), "rom");
  CHECK(rom != NULL);
  CHECK(RegionHas(rom, ".text"));
  CHECK(RegionHas(rom, ".text.hot"));
  CHECK(RegionHas(rom, ".rodata.str1"));
  CHECK(RegionHas(rom, ".bss"));
  CHECK(RegionHas(rom, "COMMON"));
  CHECK(rom->trailing_align == 8);
  CHECK(HasDiscard(&config, ".comment"));
  CHECK(HasDiscard(&config, ".note.GNU-stack"));
  CHECK(!HasDiscard(&config, ".text"));
  ConfigScriptSymbol* etext = FindSym(&config, "_etext");
  ConfigScriptSymbol* end = FindSym(&config, "_end");
  CHECK(etext != NULL && !etext->provide && !etext->image_end);
  CHECK(end != NULL && end->provide && end->image_end);
  LinkerConfigDestruct(&config);
}

static void TestExpressionsAndInclude(void) {
  const char* tmp = getenv("TEST_TMPDIR");
  char inc_path[512];
  snprintf(inc_path, sizeof(inc_path), "%s/linker_script_inc.ld",
           (tmp != NULL && tmp[0] != '\0') ? tmp : ".");
  FILE* fp = fopen(inc_path, "w");
  CHECK(fp != NULL);
  if (fp != NULL) {
    fputs("MEMORY { extra (rw!x) : ORIGIN = 0x2000, LENGTH = 4K }\n", fp);
    fclose(fp);
  }
  char script[2048];
  snprintf(script, sizeof(script),
           "INCLUDE \"%s\"\n"
           "REGION_ALIAS(\"alias_extra\", extra)\n"
           "MEMORY {\n"
           "  flash (rx) : ORIGIN = 0x1000 + (2 * 0x100), LENGTH = MAX(1K, 512)\n"
           "  ram (rwx) : ORIGIN = 0x8000, LENGTH = 2K\n"
           "}\n"
           "EXTERN(_start)\n"
           "PROVIDE(_stack = ORIGIN(ram) + LENGTH(ram));\n"
           "heap_base = 0x20 << 8;\n"
           "heap_size = ABSOLUTE(heap_base / 2);\n"
           "derived = heap_size + 1;\n"
           "SECTIONS {\n"
           "  .text ALIGN(4) SUBALIGN(4) : { KEEP(*(.text*)) } > flash\n"
           "  .data : {\n"
           "    *(SORT_BY_NAME(.data.*) EXCLUDE_FILE(*crtend.o) .data.keep)\n"
           "    *(.foo[ab] .bar?x .mid*tail)\n"
           "  } > alias_extra\n"
           "  foo = 0x40 << 4;\n"
           "}\n",
           inc_path);
  LinkerConfig config = {0};
  CHECK(LinkerScriptParseString("linker_script_main.ld", script,
                                ELF_MACHINE_TYPE_RISC_V, &config));
  ConfigRegion* flash =
      FindRegion(FindSeg(&config, kConfigSegmentTypeText), "flash");
  CHECK(flash != NULL);
  CHECK(flash->start_addr == 0x1200);
  CHECK(flash->size == 1024);
  ConfigRegion* extra =
      FindRegion(FindSeg(&config, kConfigSegmentTypeData), "extra");
  CHECK(extra != NULL);
  CHECK(extra->start_addr == 0x2000);
  CHECK(extra->size == 4096);
  CHECK(FindRegion(FindSeg(&config, kConfigSegmentTypeData), "alias_extra") ==
        NULL);
  CHECK(RegionHas(extra, ".data.sorted"));
  CHECK(RegionHas(extra, ".data.keep"));
  CHECK(RegionHas(extra, ".fooa"));
  CHECK(RegionHas(extra, ".bar1x"));
  CHECK(RegionHas(extra, ".mid.any.tail"));
  ConfigScriptSymbol* foo = FindSym(&config, "foo");
  CHECK(foo != NULL && foo->has_absolute && foo->absolute == 0x400);
  ConfigScriptSymbol* stack = FindSym(&config, "_stack");
  CHECK(stack != NULL && stack->provide && stack->has_absolute);
  CHECK(stack->absolute == 0x8000 + 2048);
  ConfigScriptSymbol* heap = FindSym(&config, "heap_base");
  CHECK(heap != NULL && heap->has_absolute && heap->absolute == 0x2000);
  ConfigScriptSymbol* heap_size = FindSym(&config, "heap_size");
  CHECK(heap_size != NULL && heap_size->has_absolute &&
        heap_size->absolute == 0x1000);
  ConfigScriptSymbol* derived = FindSym(&config, "derived");
  CHECK(derived != NULL && derived->has_absolute &&
        derived->absolute == 0x1001);
  CHECK(LinkerConfigPatternMatch(".text*", ".text.cold"));
  CHECK(LinkerConfigPatternMatch("?.text", "atext") == false);
  CHECK(LinkerConfigPatternMatch("[.]text", ".text"));
  LinkerConfigDestruct(&config);
  remove(inc_path);
}

int main(void) {
  TestStandardScript();
  Test6502RomBuiltin();
  TestIntromScript();
  TestElfBuiltins();
  TestCommentsAndHash();
  TestMissingBuiltin();
  TestWildcardsDiscardProvide();
  TestExpressionsAndInclude();
  if (g_failures != 0) {
    fprintf(stderr, "%d check(s) failed\n", g_failures);
    return 1;
  }
  return 0;
}
