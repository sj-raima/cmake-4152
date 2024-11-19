#!/usr/bin/pwsh

$basePresets = @("android", "integrity", "linux", "macos", "qnx", "vxworks", "windows", "none", "androidDev", "linuxDev", "windowsDev", "noneDev")

foreach ($basePreset in $basePresets) {
  & jq -f perl/genExtraProfiles.jq presets/${basePreset}.json.in > presets/${basePreset}.json
}

if (Test-Path -Path "CMakeUserPresets.json.in") {
    & jq -f perl/genExtraProfiles.jq CMakeUserPresets.json.in > CMakeUserPresets.json
}

