#include <iostream>
#include <fstream>
#include <sstream>
#include <locale>
#include <codecvt>
#include <algorithm>
#include <random>
#include <dirent/dirent.h>
#include "Filelist.h"

////////////////////////////////////////////////////////////////////////////////

std::wstring s2ws(const std::string& str)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.from_bytes(str);
}

std::string ws2s(const std::wstring& wstr)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}

std::wstring cwd()
{
    TCHAR buffer[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, buffer, MAX_PATH);
    std::wstring::size_type pos = std::wstring(buffer).find_last_of(L"\\/");
    return std::wstring(buffer).substr(0, pos);
}

////////////////////////////////////////////////////////////////////////////////

Filelist::Filelist(const std::string& exts)
    : m_index(0)
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
        ParsePlaylistM3U(filepath);
    }
    else if (filepath.ext() == "ayl")
    {
        ParsePlaylistAYL(filepath);
    }
    else if (filepath.hasDir() && !filepath.hasNameExt())
    {
        ParseFolder(filepath);
    }
    else
    {
        InsertItem(path);
    }
}

bool Filelist::empty() const
{
    return m_files.empty();
}

uint32_t Filelist::count() const
{
    return (uint32_t)m_files.size();
}

int32_t Filelist::index() const
{
    return m_index;
}

bool Filelist::prev(std::string& path) const
{
    if (!empty())
    {
        if (m_index >= 0)
        {
            path = m_files[m_index--].dirNameExt();
            return true;
        }
    }
    return false;
}

bool Filelist::next(std::string& path) const
{
    if (!empty())
    {
        if (m_index < int(m_files.size()))
        {
            path = m_files[m_index++].dirNameExt();
            return true;
        }
    }
    return false;
}

void Filelist::shuffle()
{
    std::random_device randomDevice;
    std::mt19937 randomGenerator(randomDevice());
    std::shuffle(m_files.begin(), m_files.end(), randomGenerator);
}

bool Filelist::IsSupportedExt(const Filepath& path)
{
    return (std::find(m_exts.begin(), m_exts.end(), path.ext()) != m_exts.end());
}

void Filelist::ParsePlaylistM3U(const Filepath& path)
{
    std::ifstream fileStream;
    fileStream.open(path.dirNameExt());

    if (fileStream)
    {
        std::string item;
        Filepath itemPath;

        while (getline(fileStream, item))
        {
            itemPath.dirNameExt(item);
            InsertItem(itemPath);
        }
        fileStream.close();
    }
}

void Filelist::ParsePlaylistAYL(const Filepath& path)
{
}

void Filelist::ParseFolder(const Filepath& path)
{
    DIR* dir;
    dirent* ent;
    Filepath itemPath;

    if (dir = opendir(path.dir().c_str()))
    {
        while (ent = readdir(dir))
        {
            if (ent->d_type == DT_REG)
            {
                itemPath.dirNameExt(path.dir() + std::string(ent->d_name));
                InsertItem(itemPath);
            }
        }
        closedir(dir);
    }
}

void Filelist::InsertItem(const Filepath& path)
{
    if (path.hasNameExt() && IsSupportedExt(path))
    {
        m_files.push_back(path);

        if (!m_files.back().hasDir())
        {
            // TODO: Unicode support
            m_files.back().dir(ws2s(cwd()));
        }
    }

    //else
    //{
    //    std::cout << path.nameExt() << std::endl;
    //}
}
