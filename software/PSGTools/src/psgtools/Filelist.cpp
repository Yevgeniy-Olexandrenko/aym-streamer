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
                ImportPlaylistM3U(path);
            }
            else if (path.extension() == ".ayl")
            {
                ImportPlaylistAYL(path);
            }
            else
            {
                ImportFile(path);
            }
        }
        else
        {
            ImportFolder(path);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

bool Filelist::IsEmpty() const
{
    return m_files.empty();
}

uint32_t Filelist::GetNumberOfFiles() const
{
    return (uint32_t)m_files.size();
}

int32_t Filelist::GetCurrFileIndex() const
{
    return m_index;
}

bool Filelist::GetPrevFile(std::filesystem::path& path) const
{
    if (!IsEmpty())
    {
        if (m_index > 0)
        {
            path = m_files[--m_index];
            return true;
        }
    }
    return false;
}

bool Filelist::GetNextFile(std::filesystem::path& path) const
{
    if (!IsEmpty())
    {
        if (m_index < int(m_files.size() - 1))
        {
            path = m_files[++m_index];
            return true;
        }
    }
    return false;
}

void Filelist::RandomShuffle()
{
    std::random_device randomDevice;
    std::mt19937 randomGenerator(randomDevice());
    std::shuffle(m_files.begin(), m_files.end(), randomGenerator);
    m_index = -1;
}

bool Filelist::ExportPlaylist(const std::filesystem::path& path)
{
    if (path.has_filename())
    {
        if (path.extension() == ".m3u")
        {
            return ExportPlaylistM3U(std::filesystem::absolute(path));
        }
        else if (path.extension() == ".ayl")
        {
            return ExportPlaylistAYL(std::filesystem::absolute(path));
        }
    }
    return false;
}

void Filelist::ImportPlaylistM3U(const std::filesystem::path& path)
{
    std::ifstream fileStream;
    fileStream.open(path);

    if (fileStream)
    {
        std::string entity;
        while (getline(fileStream, entity))
        {
            if (!entity.empty() && entity[0] != '#')
            {
                auto entityPath = ConvertToAbsolute(path, entity);
                ImportFile(entityPath);
            }
        }
        fileStream.close();
    }
}

void Filelist::ImportPlaylistAYL(const std::filesystem::path& path)
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
                    ImportFile(entityPath);
                }
            }
        }
        fileStream.close();
    }
}

void Filelist::ImportFolder(const std::filesystem::path& path)
{
    for (const auto& file : std::filesystem::directory_iterator(path))
    {
        if (std::filesystem::is_regular_file(file))
        {
            ImportFile(file);
        }
    }
}

void Filelist::ImportFile(const std::filesystem::path& path)
{
    if (path.has_filename())
    {
        auto extension = path.extension().string();
        std::for_each(extension.begin(), extension.end(), [](char& c) { c = ::tolower(c); });

        if (std::find(m_exts.begin(), m_exts.end(), extension) != m_exts.end())
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

bool Filelist::ExportPlaylistM3U(const std::filesystem::path& path)
{
    std::ofstream stream;
    stream.open(path);

    if (stream)
    {
        auto basePath = path.parent_path();
        for (const auto& filePath : m_files)
        {
            auto proximateFilePath = std::filesystem::proximate(filePath, basePath);
            stream << proximateFilePath.string() << std::endl;
        }

        stream.close();
        return true;
    }
    return false;
}

bool Filelist::ExportPlaylistAYL(const std::filesystem::path& path)
{
    std::ofstream stream;
    stream.open(path);

    if (stream)
    {
        stream << "ZX Spectrum Sound Chip Emulator Play List File v1.0" << std::endl;

        auto basePath = path.parent_path();
        for (const auto& filePath : m_files)
        {
            auto proximateFilePath = std::filesystem::proximate(filePath, basePath);
            stream << proximateFilePath.string() << std::endl;
        }

        stream.close();
        return true;
    }
    return false;
}
