## evidence_id

ev_b720422c2a69c84db42ee28bb9c9b597e79f0beb6ec196b8cbeda303b5f97991

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

| Key     | Value  |
|---------|--------|
| id      | ghidra |
| name    | Ghidra |
| version | 12.1.4 |

## analysis_profile.provider

| Key     | Value  |
|---------|--------|
| id      | ghidra |
| name    | Ghidra |
| version | 12.1.4 |

## analysis_profile.parameters.target_kind

executable

## analysis_profile.parameters.target_format

pe

## analysis_profile.parameters.executable_role

application

## analysis_profile.parameters.managed

false

## analysis_profile.parameters.native_aot_metadata

| Key                  | Value                       |
|----------------------|-----------------------------|
| contract_revision    | rtr-9.1-x64-pe-read-only-v3 |
| source_bytes         | 134217728                   |
| work_units           | 67108864                    |
| working_memory_bytes | 67108864                    |
| report_bytes         | 4194304                     |

## analysis_profile.parameters.architecture

x86

## analysis_profile.parameters.available_architectures

x86

## analysis_profile.parameters.import_mode

ephemeral-source-immutable

## analysis_profile.parameters.pdb_observations

program-info-v1

## analysis_profile.parameters.annotation_policy

atomic-function-entry-metadata-v1

## analysis_profile.parameters.load_image_observations

source-mappings-entry-context-v2

## analysis_profile.parameters.function_body_evidence

complete-inclusive-ranges-v1

## analysis_profile.parameters.function_references

complete-body-and-entry-reference-manager-v2

## analysis_profile.parameters.call_relationship_coverage

function-body-call-coverage-v5

## analysis_profile.parameters.function_dossier_coverage

observed-facets-and-call-relationships-v1

## analysis_profile.parameters.string_inventory_evidence

defined-data-coverage-v1

## analysis_profile.parameters.location_resolution

explicit-address-exact-entry-symbol-first-v3

## analysis_profile.parameters.procedure_file_offset_resolution

address-first-unique-target-source-mapping-v2

## analysis_profile.parameters.instruction_flow_evidence

decoded-return-pcode-v1

## analysis_profile.parameters.instruction_references

exact-address-incoming-v1

## analysis_profile.parameters.function_boundary_observations

ghidra-function-boundary-limitations-v3

## analysis_profile.parameters.no_return_repair

disabled-windows-p0

## analysis_profile.parameters.process_launch

official-headless-script-v1

## analysis_profile.parameters.jump_table_evidence

typed-case-default-blocks-v1

## analysis_profile.parameters.decompiler_jump_loads

true

## analysis_profile.parameters.loader

auto-from-header

## analysis_profile.parameters.language_id

auto-from-header

## analysis_profile.parameters.compiler_spec_id

auto-default

## analysis_profile.parameters.analyzer_preset

ghidra-default

## analysis_profile.digest

3c06738e45e144d5f073edbd3244c6bff46222f389525fe659bc92dca2ccdece

## predicate_type

rea.analysis

## operation

search_procedures

## parameters

| Key            | Value                              |
|----------------|------------------------------------|
| pattern        | CreateProcess|WinExec|ShellExecute |
| mode           | regex                              |
| case_sensitive | false                              |

## raw_result

| address       | value                        |
|---------------|------------------------------|
| EXTERNAL:0x2b | KERNEL32.DLL::CreateProcessA |

## normalized_result

| address       | value                        |
|---------------|------------------------------|
| EXTERNAL:0x2b | KERNEL32.DLL::CreateProcessA |

## confidence

observed

## authority

shipped-artifact

## environment

null

## limitations

The session serves operations only after Ghidra reports default auto-analysis complete; incomplete analysis does not expose partial results.,The imported Program and temporary project are ephemeral and deleted on close. Annotation operations edit session database metadata; original executable bytes are never written.,Cancelling an active Ghidra request terminates its ephemeral database and discards session annotations. The selected target remains open; the next provider query imports the original artifact again.,Default-space addresses use lowercase 0x-prefixed hexadecimal; other address spaces use <percent-encoded-space>:0x<hex>.,Regex mode uses Java Pattern semantics. Matcher or compiler stack exhaustion returns a resource constraint without closing the session; literal mode avoids regex recursion and returns complete matching values.,Windows Ghidra P0 accepts native x86 and x86-64 PE applications and DLLs, and explicitly selected raw 6502 images; managed, hostile, sensitive, and mutable-path targets are unsupported.,The Windows bridge uses authenticated IPv4 loopback because Node path-based IPC does not expose Java AF_UNIX sockets; the endpoint file contains no bearer token.,Windows sessions require the matching packaged Windows x64 native addon, local NTFS targets and runtimes, and Windows 10 or later. Native handles enforce path admission, private DACLs, and Job Object ownership automatically.

## relationship_coverage

null

## locations



## evidence_links

