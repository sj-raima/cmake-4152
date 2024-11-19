["Release", "Debug", "RelWithDebInfo", "MinSizeRel", "RelO3WithDebInfo", "RelFlto", "SanitizeAddress", "SanitizeMemory", "SanitizeThread", "SanitizeUndefined", "Valgrind", "FailSim", "FailSimAndSanitizeAddress", "FailSimAndSanitizeMemory"] as $suffixes |
{
    "version": (.version),
    "include": .include,
    "configurePresets": (.configurePresets),
    "buildPresets": [
        .configurePresets[] as $configurePreset |
        select ($configurePreset.hidden == null or $configurePreset.hidden == false ) |
        $suffixes [] as $suffix |
        {
            "name": ($configurePreset.name + $suffix),
            "displayName": $suffix,
            "configurePreset": $configurePreset.name,
            "configuration": $suffix
        }
    ],
    "packagePresets": [
        .configurePresets[] as $configurePreset |
        $suffixes [] as $suffix |
        select ($configurePreset.hidden == null or $configurePreset.hidden == false ) |
        {
            "name": ($configurePreset.name + $suffix),
            "configurePreset": $configurePreset.name,
            "configurations": [
                ($configurePreset.name + $suffix)
            ]
        }
    ],
    "testPresets": [
        .configurePresets[] as $configurePreset |
        select ($configurePreset.hidden == null or $configurePreset.hidden == false ) |
        {
            "name": ("tfs_local-" + $configurePreset.name),
            "displayName": "tfs_local",
            "configurePreset": $configurePreset.name,
            "environment": {
                "QA_ARGS": "--tfsl --exclude-feature GLOBAL_DBS"
            },
            "output":
            {
                "outputOnFailure": true
            }
        },
        {
            "name": ("rdm_client-" + $configurePreset.name),
            "displayName": "rdm_client",
            "configurePreset": $configurePreset.name,
            "environment": {
                "QA_ARGS": "--tfsm"
            },
            "output":
            {
                "outputOnFailure": true
            }
        },
        {
            "name": ("tfs_client-" + $configurePreset.name),
            "displayName": "tfs_client",
            "configurePreset": $configurePreset.name,
            "environment": {
                "QA_ARGS": "--tfsr"
            },
            "output":
            {
                "outputOnFailure": true
            }
        },
        {
            "name": ("tfs_client_shm-" + $configurePreset.name),
            "displayName": "tfs_client_shm",
            "configurePreset": $configurePreset.name,
            "environment": {
                "QA_ARGS": "--tfsr tfs=--tfs-shm://21553"
            },
            "output":
            {
                "outputOnFailure": true
            }
        },
        {
            "name": ("tfs_client_shm_poll-" + $configurePreset.name),
            "displayName": "tfs_client_shm_poll",
            "configurePreset": $configurePreset.name,
            "environment": {
                "QA_ARGS": "--tfsr tfs=--tfs-shm-poll://"
            },
            "output":
            {
                "outputOnFailure": true
            }
        },
        {
            "name": ("tfs_client_tcp-" + $configurePreset.name),
            "displayName": "tfs_client_tcp",
            "configurePreset": $configurePreset.name,
            "environment": {
                "QA_ARGS": "--tfsr tfs=--tfs-tcp://localhost:21553"
            },
            "output":
            {
                "outputOnFailure": true
            }
        }
    ]
}
| with_entries (if .key == "include" and .value == null then empty else . end)
