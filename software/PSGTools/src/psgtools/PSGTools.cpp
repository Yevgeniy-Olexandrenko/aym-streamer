#include <iostream>
#include <chrono>
#include <Windows.h>
#include <sstream>
#include <fstream>

#undef max

#include "PSGLib.h"
#include "Filelist.h"
#include "interface/ConsoleGUI.h"

const std::filesystem::path k_favoritesPath = "favorites.m3u";
const std::string k_supportedFileTypes = "sqt|ym|stp|vgz|vgm|asc|stc|pt2|pt3|psg|vtx";

std::shared_ptr<Filelist> m_filelist;
std::shared_ptr<Filelist> m_favorites;

std::shared_ptr<Output>   m_output;
std::shared_ptr<Player>   m_player;

static BOOL WINAPI console_ctrl_handler(DWORD dwCtrlType)
{
    if (m_player)
    {
        m_player->Stop();
    }
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////

struct FrameRefs
{
    const Frame* prev = nullptr;
    const Frame* next = nullptr;
};

std::vector<FrameRefs> m_framesRefs;

bool IsSameFrames(const Frame& f1, const Frame& f2)
{
    for (int chip = 0; chip < 2; ++chip)
    {
        for (Register reg = BankA_Fst; reg <= BankA_Lst; ++reg)
        {
            if (f1.Read(chip, reg) != f2.Read(chip, reg)) return false;
        }

        for (Register reg = BankB_Fst; reg <= BankB_Lst; ++reg)
        {
            if (f1.Read(chip, reg) != f2.Read(chip, reg)) return false;
        }
    }
    return true;
}

void ComputeFrameRefs(const Stream& stream)
{
    int count = stream.framesCount();
    int depth = count;// 32;

    m_framesRefs.resize(count);
    for (int frameId = 1; frameId < count; ++frameId)
    {
        for (int otherId = frameId - 1, lastId = std::max(frameId - depth, 0); otherId >= lastId; --otherId)
        {
            const Frame& frame = stream.GetFrame(frameId);
            const Frame& other = stream.GetFrame(otherId);

            if (IsSameFrames(frame, other))
            {
                m_framesRefs[frameId].prev = &other;
                m_framesRefs[otherId].next = &frame;
                break;
            }
        }
    }

    std::ofstream debug_out;
    debug_out.open("test.txt");
    if (debug_out)
    {
        int unique = 0;
        int count = m_framesRefs.size();

        for (int frameId = 1; frameId < count; ++frameId)
        {
            if (m_framesRefs[frameId].next)
                debug_out << std::setw(5) << std::setfill('0') << int(m_framesRefs[frameId].next->GetId());
            else
                debug_out << "-----";

            debug_out << " -> " << std::setw(5) << std::setfill('0') << frameId << " -> ";

            if (m_framesRefs[frameId].prev)
                debug_out << std::setw(5) << std::setfill('0') << int(m_framesRefs[frameId].prev->GetId());
            else
                debug_out << "-----";
            debug_out << "\n";

            if (!m_framesRefs[frameId].prev) unique++;
        }
        debug_out << "duplicates: " << int(100.f * (count - unique) / count + 0.5f) << "%\n";
        debug_out.close();
    }
}

////////////////////////////////////////////////////////////////////////////////

void PrintWellcome()
{
    gui::Init(L"PSG Tools");

    using namespace terminal;
    std::cout << ' ' << color::bright_blue << std::string(gui::k_consoleWidth - 2, '-') << std::endl;
    std::cout << ' ' << color::bright_red << "PSG Tools v1.0" << std::endl;
    std::cout << ' ' << color::bright_red << "by Yevgeniy Olexandrenko" << std::endl;
    std::cout << ' ' << color::bright_blue << std::string(gui::k_consoleWidth - 2, '-') << std::endl;
    std::cout << color::reset << std::endl;
    std::cout << " ENTER\t- Pause/Resume current song playback" << std::endl;
    std::cout << " DOWN\t- Go to next song in playlist" << std::endl;
    std::cout << " UP\t- Go to previous song in playlist" << std::endl;
    std::cout << " F\t- Add/Remove current song to/from favorites" << std::endl;
    std::cout << color::reset << std::endl;
}

void PlayInputFiles()
{
    ////////////////////////////////////////////////////////////////////////////
#if 0
    const int k_comPortIndex = 4;
    m_output.reset(new Streamer(k_comPortIndex));
#else
    m_output.reset(new Emulator());
#endif
    m_player.reset(new Player(*m_output));
#if 0
    m_filelist->RandomShuffle();
#endif
    ////////////////////////////////////////////////////////////////////////////

    bool goToPrev = false;
    std::filesystem::path path;

    while (goToPrev ? m_filelist->GetPrevFile(path) : m_filelist->GetNextFile(path))
    {
        Stream stream;
#if 0
        //stream.chip.first.model(Chip::Model::AY8930);
        //stream.chip.second.model(Chip::Model::YM2149);
        //stream.chip.output(Chip::Output::Stereo);
        //stream.chip.clockValue(1500000);
#endif
        if (PSG::Decode(path, stream))
        {
            //ComputeFrameRefs(stream);


            goToPrev = false; // if decoding OK, move to next by default

            size_t staticHeight  = 0;
            size_t dynamicHeight = 0;

            if (m_player->Init(stream))
            {
                bool printStatic = true;
                FrameId frameId  = -1;

                m_player->Play();
                while (m_player->IsPlaying())
                {
                    gui::Update();
                    if (m_player->GetFrameId() != frameId)
                    {
                        frameId = m_player->GetFrameId();

                        terminal::cursor::move_up(dynamicHeight);
                        dynamicHeight = 0;

                        if (printStatic)
                        {
                            gui::Clear(staticHeight);
                            staticHeight = 0;

                            auto index = m_filelist->GetCurrFileIndex();
                            auto amount = m_filelist->GetNumberOfFiles();
                            auto favorite = m_favorites->ContainsFile(stream.file);

                            staticHeight += gui::PrintInputFile(stream, index, amount, favorite);
                            staticHeight += gui::PrintStreamInfo(stream, *m_output);
                            printStatic = false;
                        }

                        dynamicHeight += gui::PrintStreamFrames(stream, frameId);
                        dynamicHeight += gui::PrintPlaybackProgress(stream, frameId);
                    }

                    if (gui::GetKeyState(VK_UP).pressed)
                    {
                        if (!m_player->IsPaused())
                        {
                            m_player->Stop();
                            goToPrev = true;
                            break;
                        }
                    }

                    if (gui::GetKeyState(VK_DOWN).pressed)
                    {
                        if (!m_player->IsPaused())
                        {
                            m_player->Stop();
                            goToPrev = false;
                            break;
                        }
                    }

                    if (gui::GetKeyState(VK_RETURN).pressed)
                    {
                        if (m_player->IsPaused())
                            m_player->Play();
                        else
                            m_player->Stop();
                    }

                    if (gui::GetKeyState('F').pressed)
                    {
                        if (m_favorites->ContainsFile (stream.file)
                            ? m_favorites->EraseFile  (stream.file)
                            : m_favorites->InsertFile (stream.file))
                        {
                            m_favorites->ExportPlaylist(k_favoritesPath);
                            printStatic = true;   
                        }
                    }

                    Sleep(1);
                }

                gui::Clear(dynamicHeight);
            }
            else
            {
                std::cout << "Could not init player with module" << std::endl;
            }
        }
        else
        {
            std::cout << "Could not decode file: " << path << std::endl;
        }
        path.clear();
    }
}

void ConvertInputFiles(const std::filesystem::path& outputPath)
{
    std::filesystem::path path;
    while (m_filelist->GetNextFile(path))
    {
        Stream stream;
        if (PSG::Decode(path, stream))
        {
            if (PSG::Encode(outputPath, stream))
            {
                std::cout << "Done file encoding: " << outputPath << std::endl;
            }
            else
            {
                std::cout << "Could not encode file: " << outputPath << std::endl;
            }
        }
        else
        {
            std::cout << "Could not decode file: " << path << std::endl;
        }
    }
}

int main(int argc, char* argv[])
{
    bool isConverting = false;
    std::filesystem::path inputPath;
    std::filesystem::path outputPath;

    // setup input path
    if (argc > 1)
        inputPath = argv[1];
    else
        inputPath = k_favoritesPath;
    
    // setup output path
    if (argc > 2)
    {
        outputPath = argv[2];
        isConverting = true;
    }

    // setup file lists
    m_filelist.reset(new Filelist(k_supportedFileTypes, inputPath));
    m_favorites.reset(new Filelist(k_supportedFileTypes, k_favoritesPath));

    PrintWellcome();
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);

    if (!m_filelist->IsEmpty())
    {
        if (isConverting)
        {
            ConvertInputFiles(outputPath);
        }
        else
        {
            PlayInputFiles();
        }
    }
    return 0;
}
