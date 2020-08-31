Preamble: This is the product of researching mach-o files for core file analysis. The current documentation for these files is missing critical information of the layout. I intend on unifying that information here, with accompanying code which would hopefully aid corefile breakdown. 

##  General References:
Basic Mach-O structure: [aidansteeles OS X ABI Mach-O File Format Reference](https://github.com/aidansteele/osx-abi-macho-file-format-reference)
Official Mac source code: [loader.h](https://opensource.apple.com/source/cctools/cctools-795/include/mach-o/loader.h)

# MachO-MH_CORE-ABI

This document provides the file structure of the Mach-O file with a MH_CORE filetype. The in depth structure of the Mach-O file in general may be found in [aidansteeles guide](https://github.com/aidansteele/osx-abi-macho-file-format-reference), while the reference code may be found in [Apple's OpenSource code repository](https://opensource.apple.com/source/cctools/cctools-795/include/mach-o/loader.h). While the general Mach-O structure will be referenced, in depth details are outside the scope of this document. 


