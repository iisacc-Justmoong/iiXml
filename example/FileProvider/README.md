# iiXml file-provider consumer

This separately configured C++23 consumer supplies `iiXml::IO::FileReader`
using iiFileProvider. Build the SDKs independently, then configure this project
under the iiXml `build/` directory with both package prefixes in
`CMAKE_PREFIX_PATH`. Run its `iiXml.FileProvider` contract test with CTest.
The provider owns the temporary test directory and all test file creation.

Default-constructed Qt `FileParser` and `GetFile` have no reader; pass
`fileProviderReader()` to each object. Captured application resources must
outlive the reader and provide any needed synchronization.
