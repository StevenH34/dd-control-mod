## evidence_id

ev_e9e07d8e884665102584de7de89f687ed7b3e14ad1f8de3a83ca3b101c4f247f

## subject.name

DDSTART.EXE

## subject.digest

| Key    | Value                                                            |
|--------|------------------------------------------------------------------|
| sha256 | c45d23a17b29057f33c020f2b5853fd19fbce31b3ccae66af99dc3fd4c0d8dfb |

## subject.local_path

<game folder>\DDSTART.EXE

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

application

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

3c06738e45e144d5f073edbd3244c6bff46222f389525fe659bc92dca2ccdece

## analysis_profile.parameters.workflow_revision

3

## analysis_profile.digest

f6e648d2d6b6da88dcf26ba1d744e1e5b2f45f623f8c5408d6783dd66ec3f8ca

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

target-c45d23a17b29.EXE

## normalized_result.image_base

0x400000

## normalized_result.segments

| name    | start    | end      | length | image_base | address_space |
|---------|----------|----------|--------|------------|---------------|
| Headers | 0x400000 | 0x400400 | 1024   | 0x400000   | ram           |
| .text   | 0x401000 | 0x406a00 | 23040  | 0x400000   | ram           |
| .rdata  | 0x407000 | 0x407400 | 1024   | 0x400000   | ram           |
| .data   | 0x408000 | 0x40a654 | 9812   | 0x400000   | ram           |
| .idata  | 0x40b000 | 0x40b800 | 2048   | 0x400000   | ram           |
| .rsrc   | 0x40c000 | 0x40d000 | 4096   | 0x400000   | ram           |
| .reloc  | 0x40d000 | 0x40d800 | 2048   | 0x400000   | ram           |

## normalized_result.segment_count

7

## normalized_result.procedure_count

156

## normalized_result.string_count

92

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

