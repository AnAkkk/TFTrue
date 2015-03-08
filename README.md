1) How to compile

For now, the project uses qmake which means it needs to be opened in Qt Creator.
Is it compiled with MSVC 2013 on Windows and gcc 4.9.2 on Linux but it might work with older and/or newer versions.

You will need to adjust SOURCE_DIR in TFTrue.pro to point to the source SDK 2013 directory (https://github.com/ValveSoftware/source-sdk-2013).

2) Additional dependencies
ModuleScanner and FunctionRoute were made by Didrole. The source code is not available yet but should be *soon*.
The libraries are part of the repository for now.
jsoncpp (https://github.com/open-source-parsers/jsoncpp) is needed too and included in this repository.
