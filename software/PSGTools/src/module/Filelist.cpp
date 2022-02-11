#include <sstream>
#include "Filelist.h"
#include "Filepath.h"

Filelist::Filelist(const std::string& exts)
{
    std::string ext;
    std::stringstream ss(exts);
    
    while (getline(ss, ext, '|')) 
    {
        m_exts.push_back(ext);
    }
}

Filelist::Filelist(const std::string& exts, const std::string& path)
    : Filelist(exts)
{
    Filepath filepath(path);
    if (filepath.ext() == "m3u")
    {
        // TODO
    }
    else if (filepath.ext() == "ayl")
    {
        // TODO
    }
}
