#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include "Filelist.h"

std::filesystem::path ConvertToAbsolute(const std::filesystem::path& base, const std::filesystem::path& path)
{
    if (base.is_absolute() && path.is_relative())
    {
        std::filesystem::path absolute = base;
        absolute.replace_filename(path);
        return absolute.lexically_normal();
    }
    return path;
}

////////////////////////////////////////////////////////////////////////////////

Filelist::Filelist(const std::string& exts)
    : m_index(-1)
{
    std::string ext;
    std::stringstream ss(exts);
    
    while (getline(ss, ext, '|')) 
    {
        m_exts.push_back("." + ext);
    }
}

Filelist::Filelist(const std::string& exts, const std::filesystem::path& path)
    : Filelist(exts)
{
    for (auto path : glob::glob(path.string()))
    {
        if (path.is_relative())
        {
            path = std::filesystem::absolute(path);
        }

        if (path.has_filename())
        {
            if (path.extension() == ".m3u")
            {
                ParsePlaylistM3U(path);
            }
            else if (path.extension() == ".ayl")
            {
                ParsePlaylistAYL(path);
            }
            else
            {
                InsertPath(path);
            }
        }
        else
        {
            ParseFolder(path);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

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

bool Filelist::prev(std::filesystem::path& path) const
{
    if (!empty())
    {
        if (m_index > 0)
        {
            path = m_files[--m_index];
            return true;
        }
    }
    return false;
}

bool Filelist::next(std::filesystem::path& path) const
{
    if (!empty())
    {
        if (m_index < int(m_files.size() - 1))
        {
            path = m_files[++m_index];
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
    m_index = -1;
}

void Filelist::ParsePlaylistM3U(const std::filesystem::path& path)
{
    std::ifstream fileStream;
    fileStream.open(path);

    if (fileStream)
    {
        std::string entity;
        while (getline(fileStream, entity))
        {
            auto entityPath = ConvertToAbsolute(path, entity);
            InsertPath(entityPath);
        }
        fileStream.close();
    }
}

void Filelist::ParsePlaylistAYL(const std::filesystem::path& path)
{
    std::ifstream fileStream;
    fileStream.open(path);

    if (fileStream)
    {
        std::string entity;
        bool skipLine = false;
        while (getline(fileStream, entity))
        {
            if (!entity.empty())
            {
                if (entity == "<") skipLine = true;
                else if (entity == ">") skipLine = false;
                else if (!skipLine)
                {
                    auto entityPath = ConvertToAbsolute(path, entity);
                    InsertPath(entityPath);
                }
            }
        }
        fileStream.close();
    }
}

void Filelist::ParseFolder(const std::filesystem::path& path)
{
    for (const auto& file : std::filesystem::directory_iterator(path))
    {
        if (std::filesystem::is_regular_file(file))
        {
            InsertPath(file);
        }
    }
}

void Filelist::InsertPath(const std::filesystem::path& path)
{
    if (path.has_filename())
    {
        if (std::find(m_exts.begin(), m_exts.end(), path.extension()) != m_exts.end())
        {
            if (std::filesystem::exists(path))
            {
                if (m_hashes.insert(std::filesystem::hash_value(path)).second)
                {
                    m_files.push_back(path);
                }
            }
        }
    }
}
