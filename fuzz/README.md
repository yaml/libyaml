# libyaml fuzzing harnesses

This directory contains experimental libFuzzer harnesses for several libyaml APIs.

## Targets

fuzz_scan.cpp  
Targets yaml_parser_scan().  
Exercises the lower-level scanner/tokenization layer.

fuzz_parse.cpp  
Targets yaml_parser_parse().  
Exercises the parser event pipeline and reaches code in reader.c, scanner.c, and parser.c.

fuzz_load.cpp  
Targets yaml_parser_load().  
Extends parser-side exploration into document composition and loader logic, including loader.c.

fuzz_emit.cpp  
Targets yaml_emitter_emit() using simple valid event streams.  
This is the initial emitter-side harness.

fuzz_emit_nested.cpp  
Targets yaml_emitter_emit() using nested valid event streams.  
This improves emitter-side exploration by generating more structured mappings and sequences.

fuzz_roundtrip_parse_emit.cpp  
Targets a parse → emit round-trip workflow.  
The harness parses fuzz input into events using yaml_parser_parse() and then emits those events using yaml_emitter_emit().  
This helps explore parser/emitter interaction and significantly improves emitter coverage.

## Example build

Build libyaml first, then compile a harness like:

clang++ fuzz/fuzz_parse.cpp src/.libs/libyaml.a -I include -fsanitize=fuzzer,address,undefined -g -O1 -fno-omit-frame-pointer -o fuzz_parse

Other examples:

clang++ fuzz/fuzz_scan.cpp src/.libs/libyaml.a -I include -fsanitize=fuzzer,address,undefined -g -O1 -fno-omit-frame-pointer -o fuzz_scan  
clang++ fuzz/fuzz_load.cpp src/.libs/libyaml.a -I include -fsanitize=fuzzer,address,undefined -g -O1 -fno-omit-frame-pointer -o fuzz_load  
clang++ fuzz/fuzz_emit.cpp src/.libs/libyaml.a -I include -fsanitize=fuzzer,address,undefined -g -O1 -fno-omit-frame-pointer -o fuzz_emit  
clang++ fuzz/fuzz_emit_nested.cpp src/.libs/libyaml.a -I include -fsanitize=fuzzer,address,undefined -g -O1 -fno-omit-frame-pointer -o fuzz_emit_nested  
clang++ fuzz/fuzz_roundtrip_parse_emit.cpp src/.libs/libyaml.a -I include -fsanitize=fuzzer,address,undefined -g -O1 -fno-omit-frame-pointer -o fuzz_roundtrip_parse_emit  

## Example run

./fuzz_parse corpus_dir

Other examples:

./fuzz_scan corpus_dir  
./fuzz_load corpus_dir  
./fuzz_emit corpus_dir  
./fuzz_emit_nested corpus_dir  
./fuzz_roundtrip_parse_emit corpus_dir  

## Coverage observations

Local coverage measurements with llvm-cov showed the following.

Parser-side coverage

yaml_parser_parse()
- parser.c: 92.90% line coverage
- reader.c: 94.85% line coverage
- scanner.c: 94.01% line coverage

Overall parse coverage binary
- total line coverage: 77.07%
- total branch coverage: 72.13%
- total region coverage: 78.25%

yaml_parser_load()
- loader.c: 89.55% line coverage
- parser.c: 91.85% line coverage
- reader.c: 94.85% line coverage
- scanner.c: 94.01% line coverage

Overall load coverage binary
- total line coverage: 77.56%
- total branch coverage: 71.80%
- total region coverage: 78.34%

Emitter-side coverage

Initial yaml_emitter_emit() harness
- emitter.c: 59.31% line coverage
- writer.c: 35.44% line coverage

Overall emit coverage binary
- total line coverage: 50.75%
- total branch coverage: 44.94%
- total region coverage: 50.69%

Nested emitter harness
- emitter.c: 68.65% line coverage
- writer.c: 25.32% line coverage

Overall nested emitter coverage binary
- total line coverage: 58.43%
- total branch coverage: 52.12%
- total region coverage: 58.67%

Parse → emit round-trip harness
- emitter.c: 85.35% line coverage
- writer.c: 25.32% line coverage
- parser.c: 92.90% line coverage
- reader.c: 94.85% line coverage
- scanner.c: 94.01% line coverage

Overall round-trip coverage binary
- total line coverage: 79.64%
- total branch coverage: 71.51%
- total region coverage: 79.55%

## Corpus

During local fuzzing runs, seed corpora were generated and minimized using libFuzzer's merge mode.

These corpora are not included in this repository because they are large and mostly machine-generated. The harnesses are designed to work with any YAML seed corpus.

## Summary

The strongest parser-side harnesses are fuzz_parse.cpp and fuzz_load.cpp.  
The strongest emitter-side harness is fuzz_roundtrip_parse_emit.cpp.

In local testing, the round-trip harness substantially improved emitter.c coverage compared with the initial standalone emitter harness.

## Notes

During local experimentation, an earlier round-trip harness using a fixed-size emitter output buffer triggered a double-free candidate during cleanup. After replacing that output path with a sink callback, the issue no longer reproduced, so it is not treated as a confirmed libyaml vulnerability.

These harnesses are intended as a starting point for continued fuzzing and may also be useful for future OSS-Fuzz-style integration.
