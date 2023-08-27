/* 

    Copyright (c) 2022 Paul Walker
                       Timo Kaluza (defiantnerd)
                       

    This file is part of the clap-wrappers project which is released under MIT License.
    See file LICENSE or go to https://github.com/defiantnerd/clap-wrapper for full license details.

*/

#include <vector>
#include <filesystem>

#include <Foundation/Foundation.h>

namespace Clap
{
    std::vector<std::filesystem::path> getMacCLAPSearchPaths() {
       auto res = std::vector<std::filesystem::path>();
       auto *fileManager = [NSFileManager defaultManager];
       auto *userLibURLs = [fileManager URLsForDirectory:NSLibraryDirectory
                                               inDomains:NSUserDomainMask];
       auto *sysLibURLs = [fileManager URLsForDirectory:NSLibraryDirectory
                                              inDomains:NSLocalDomainMask];

       if (userLibURLs) {
          auto *u = [userLibURLs objectAtIndex:0];
          auto p =
                  std::filesystem::path{[u fileSystemRepresentation]} / "Audio" / "Plug-Ins" / "CLAP";
          res.push_back(p);
       }

       if (sysLibURLs) {
          auto *u = [sysLibURLs objectAtIndex:0];
          auto p =
                  std::filesystem::path{[u fileSystemRepresentation]} / "Audio" / "Plug-Ins" / "CLAP";
          res.push_back(p);
       }
       return res;
    }
}