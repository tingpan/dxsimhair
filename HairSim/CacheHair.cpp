#include <exception>
#include <string>
#include "CacheHair.h"


namespace WR
{
    const float max_fps = 0.03;

    CacheHair::~CacheHair()
    {
        SAFE_DELETE_ARRAY(position);
    }

    void CacheHair::BinaryHelper::init(size_t &nf, size_t &np)
    {
        char bytes[4];
        file.read(bytes, 4);
        nf = *reinterpret_cast<int*>(bytes);

        file.read(bytes, 4);
        np = *reinterpret_cast<int*>(bytes);
    }

    void CacheHair::BinaryHelper::readFrame20(float* rigidTrans, float* pos, float* dir, size_t np)
    {
        char* bytes = reinterpret_cast<char*>(rigidTrans);
        file.read(bytes, sizeof(float)*16);

        bytes = reinterpret_cast<char*>(pos);
        file.read(bytes, sizeof(float)*np * 3);

        bytes = reinterpret_cast<char*>(dir);
        file.read(bytes, sizeof(float)*np * 3);
    }

    void CacheHair::BinaryHelper::readFrame(float* pos, size_t np)
    {
        char* bytes = reinterpret_cast<char*>(pos);
        file.read(bytes, sizeof(float)*np*3);
    }

    bool CacheHair::BinaryHelper::hasNextFrame(size_t &id)
    {
        char bytes[4];
        file.read(bytes, 4);

        if (file.eof())
            return false;

        id = *reinterpret_cast<int*>(bytes);
        return true;
    }

    void CacheHair::AsciiHelper::init(size_t &nf, size_t &np)
    {
        file >> nf;
        file >> np;
    }
    void CacheHair::AsciiHelper::readFrame(float* pos, size_t np)
    {
        std::string line;
        for (size_t i = 0; i < np; i++)
        {
            for (size_t j = 0; j < 3; j++)
                file >> pos[3 * i + j];
        }
    }
    bool CacheHair::AsciiHelper::hasNextFrame(size_t &id)
    {
        std::string line;
        std::getline(file, line);

        do
        {
            std::getline(file, line);
        } while (!file.eof() && line.substr(0, 6) != "Frame ");

        if (file.eof()) return false;

        std::string numberStr = line.substr(6, line.size());
        id = std::atoi(numberStr.c_str());
        return true;
    }

    bool CacheHair::loadFile(const char* fileName, bool binary)
    {
        if (binary)
        {
            file = std::ifstream(fileName, std::ios::binary);
            helper = new BinaryHelper(file);
        }
        else
        {
            file = std::ifstream(fileName);
            helper = new AsciiHelper(file);
        }
        if (!file.is_open()) throw std::exception("file not found!");

        helper->init(m_nFrame, m_nParticle);

        position = new float[3 * m_nParticle];
        firstFrame = file.tellg();
        bNextFrame = true;
        return true;
    }

    void CacheHair::rewind()
    {
        file.clear();
        file.seekg(firstFrame);
        set_curFrame(0);
    }

    size_t CacheHair::getFrameNumber() const
    {
        return get_nFrame();
    }

    size_t CacheHair::getCurrentFrame() const
    {
        return get_curFrame();
    }

    size_t CacheHair::n_strands() const
    {
        return get_nParticle() / N_PARTICLES_PER_STRAND;
    }

    const float* CacheHair::get_visible_particle_position(size_t i, size_t j) const
    {
        return position + (i*N_PARTICLES_PER_STRAND+j)*3;
    }

    void CacheHair::onFrame(Mat3 world, float fTime, float fTimeElapsed, void*)
    {
        timeBuffer += fTimeElapsed;
        if (timeBuffer > max_fps)
        {
            bNextFrame = true;
            timeBuffer = 0.f;
        }

        if (bNextFrame)
        {
            if (hasNextFrame())
                readFrame();
            else
            {
                rewind();
                hasNextFrame();
                readFrame();
            }
            bNextFrame = false;
        }
    }

    void CacheHair::readFrame()
    {
        helper->readFrame(position, get_nParticle());
        set_curFrame(get_curFrame() + 1);
    }

    bool CacheHair::hasNextFrame()
    {
        return helper->hasNextFrame(m_curFrame);
    }

    bool CacheHair20::loadFile(const char* fileName, bool binary)
    {
        bool result = CacheHair::loadFile(fileName, binary);
        if (result)
        {
            direction = new float[3 * get_nParticle()];
            rigidTrans = new float[16];
        }
        return result;
    }

    CacheHair20::~CacheHair20()
    {
        SAFE_DELETE_ARRAY(direction);
        SAFE_DELETE_ARRAY(rigidTrans);
    }

    void CacheHair20::readFrame()
    { 
        helper->readFrame20(rigidTrans, position, direction, get_nParticle());
        set_curFrame(get_curFrame() + 1);
    }

    const float* CacheHair20::get_visible_particle_direction(size_t i, size_t j) const
    {
        return direction + (i*N_PARTICLES_PER_STRAND + j) * 3;
    }

    const float* CacheHair20::get_rigidMotionMatrix() const
    {
        return rigidTrans;
    }

    void  CacheHair::jumpTo(int frameNo)
    {
        set_curFrame(frameNo);
        jumpTo();
    }
    
    void CacheHair20::jumpTo()
    {
        file.seekg(firstFrame + std::streamoff(get_curFrame()*(sizeof(int)+
            sizeof(float)*(16 + 3 * 2 * get_nParticle()))));
        if (hasNextFrame())
            helper->readFrame20(rigidTrans, position, direction, get_nParticle());
    }

}