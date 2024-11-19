{
  "version": 6,
  "configurePresets": [
    {
      "name": "poky",
      "description": "sj: Cross compile for Linux on SCM board provided by Cummins (A-core)",
      "inherits": [
        "cummins-aarch64-poky-linux"
      ],
      "cacheVariables": {
        "NATIVE_DIR": "${sourceDir}/b-devel"
      }
    }
  ]
}
