## evidence_id

ev_06a0e0b757ecf05434fbc530ba29f70bd3603f305b4ce4301117c2b086289db3

## subject.name

DUNGEON.DLL

## subject.digest

| Key    | Value                                                            |
|--------|------------------------------------------------------------------|
| sha256 | 95fe9ce0fff387f00704548f152e4340815213fcb3833dbe1b5c42871e7d2e56 |

## subject.local_path

<game folder>\DUNGEON.DLL

## subject.format

pe

## subject.architecture

x86

## provider

| Key     | Value                               |
|---------|-------------------------------------|
| id      | rea-workflow                        |
| name    | REA composed investigation workflow |
| version | 1                                   |

## analysis_profile.provider

| Key     | Value                               |
|---------|-------------------------------------|
| id      | rea-workflow                        |
| name    | REA composed investigation workflow |
| version | 1                                   |

## analysis_profile.parameters.upstream_analysis_profile.provider

| Key     | Value  |
|---------|--------|
| id      | ghidra |
| name    | Ghidra |
| version | 12.1.4 |

## analysis_profile.parameters.upstream_analysis_profile.parameters.target_kind

executable

## analysis_profile.parameters.upstream_analysis_profile.parameters.target_format

pe

## analysis_profile.parameters.upstream_analysis_profile.parameters.executable_role

shared-library

## analysis_profile.parameters.upstream_analysis_profile.parameters.managed

false

## analysis_profile.parameters.upstream_analysis_profile.parameters.native_aot_metadata

| Key                  | Value                       |
|----------------------|-----------------------------|
| contract_revision    | rtr-9.1-x64-pe-read-only-v3 |
| source_bytes         | 134217728                   |
| work_units           | 67108864                    |
| working_memory_bytes | 67108864                    |
| report_bytes         | 4194304                     |

## analysis_profile.parameters.upstream_analysis_profile.parameters.architecture

x86

## analysis_profile.parameters.upstream_analysis_profile.parameters.available_architectures

x86

## analysis_profile.parameters.upstream_analysis_profile.parameters.import_mode

ephemeral-source-immutable

## analysis_profile.parameters.upstream_analysis_profile.parameters.pdb_observations

program-info-v1

## analysis_profile.parameters.upstream_analysis_profile.parameters.annotation_policy

atomic-function-entry-metadata-v1

## analysis_profile.parameters.upstream_analysis_profile.parameters.load_image_observations

source-mappings-entry-context-v2

## analysis_profile.parameters.upstream_analysis_profile.parameters.function_body_evidence

complete-inclusive-ranges-v1

## analysis_profile.parameters.upstream_analysis_profile.parameters.function_references

complete-body-and-entry-reference-manager-v2

## analysis_profile.parameters.upstream_analysis_profile.parameters.call_relationship_coverage

function-body-call-coverage-v5

## analysis_profile.parameters.upstream_analysis_profile.parameters.function_dossier_coverage

observed-facets-and-call-relationships-v1

## analysis_profile.parameters.upstream_analysis_profile.parameters.string_inventory_evidence

defined-data-coverage-v1

## analysis_profile.parameters.upstream_analysis_profile.parameters.location_resolution

explicit-address-exact-entry-symbol-first-v3

## analysis_profile.parameters.upstream_analysis_profile.parameters.procedure_file_offset_resolution

address-first-unique-target-source-mapping-v2

## analysis_profile.parameters.upstream_analysis_profile.parameters.instruction_flow_evidence

decoded-return-pcode-v1

## analysis_profile.parameters.upstream_analysis_profile.parameters.instruction_references

exact-address-incoming-v1

## analysis_profile.parameters.upstream_analysis_profile.parameters.function_boundary_observations

ghidra-function-boundary-limitations-v3

## analysis_profile.parameters.upstream_analysis_profile.parameters.no_return_repair

disabled-windows-p0

## analysis_profile.parameters.upstream_analysis_profile.parameters.process_launch

official-headless-script-v1

## analysis_profile.parameters.upstream_analysis_profile.parameters.jump_table_evidence

typed-case-default-blocks-v1

## analysis_profile.parameters.upstream_analysis_profile.parameters.decompiler_jump_loads

true

## analysis_profile.parameters.upstream_analysis_profile.parameters.loader

auto-from-header

## analysis_profile.parameters.upstream_analysis_profile.parameters.language_id

auto-from-header

## analysis_profile.parameters.upstream_analysis_profile.parameters.compiler_spec_id

auto-default

## analysis_profile.parameters.upstream_analysis_profile.parameters.analyzer_preset

ghidra-default

## analysis_profile.parameters.upstream_analysis_profile.digest

64f38a45818a3d9e2f85d007cde25a1861ee4535051e38c48afd92f7e069136a

## analysis_profile.parameters.workflow_revision

3

## analysis_profile.digest

9fd934bc40d049ac4716b2202867d4ec33d42a46b6927b3aec712f562a52e771

## predicate_type

rea.analysis

## operation

binary_overview

## parameters

| Key | Value |
|-----|-------|


## raw_result

null

## normalized_result.document

target-95fe9ce0fff3.DLL

## normalized_result.image_base

0x10000000

## normalized_result.segments

| name    | start      | end        | length  | image_base | address_space |
|---------|------------|------------|---------|------------|---------------|
| Headers | 0x10000000 | 0x10000400 | 1024    | 0x10000000 | ram           |
| .text   | 0x10001000 | 0x100bac00 | 760832  | 0x10000000 | ram           |
| .rdata  | 0x100bb000 | 0x100bc800 | 6144    | 0x10000000 | ram           |
| .data   | 0x100bd000 | 0x103515e8 | 2704872 | 0x10000000 | ram           |
| .idata  | 0x10352000 | 0x10353600 | 5632    | 0x10000000 | ram           |
| .reloc  | 0x10354000 | 0x10366600 | 75264   | 0x10000000 | ram           |

## normalized_result.segment_count

6

## normalized_result.procedure_count

3643

## normalized_result.string_count

2703

## confidence

derived

## authority

shipped-artifact

## environment

null

## limitations

Derived by an REA composed workflow.,A headless Ghidra session contains exactly one imported Program, unlike Hopper's multi-document GUI session.,Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.,Charset is reported, while a non-missing terminator cannot distinguish a present terminator from a fixed or Pascal layout.,Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.,External functions and local thunks are distinct; procedure metadata identifies both and preserves a thunk target when Ghidra resolves one.,Memory-block end addresses are exclusive; permissions come from Ghidra MemoryBlock flags rather than inference from section names.,Only Ghidra-defined string Data is observed; packed or undefined string bytes may be absent. Completeness applies to that inventory, and an empty search does not establish that text is absent from the artifact. If an address is known, use read_bytes and xrefs to inspect bytes and references.,The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.,The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.,The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.,Windows Ghidra P0 accepts native x86 and x86-64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.,Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.

## relationship_coverage

null

## locations



## evidence_links

