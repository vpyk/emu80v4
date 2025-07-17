/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2023
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Crt8275.h

#ifndef CRT8275_H
#define CRT8275_H

#include "EmuObjects.h"

class Dma8257;
class PlatformCore;


/*enum CursorFormat
{
    CF_BLINKINGBLOCK = 0,
    CF_BLINKINGUNDERLINE = 1,
    CF_BLOCK = 2,
    CF_UNDERLINE = 3
};*/



struct SymbolAttributes {
    //bool vsp;    // video suppression (per symbol)
    bool rvv;    // reverse video
    bool hglt;   // highlight
    bool gpa0;   // general purpose 1
    bool gpa1;   // general purpose 2
    bool la1;    // simplified LA1, actually should be per line basis
};



struct SymbolLineAttributes {
    int lc;      // line counter
    bool vsp;    // video suppression
    bool lten;   // light enable
};



struct Symbol {
    uint8_t chr;
    SymbolAttributes symbolAttributes;
    SymbolLineAttributes symbolLineAttributes[16];
};


struct Frame {
    int nRows;          // line number from top
    int nCharsPerRow;
    int nLines;
    int underlinePos;
    bool isOffsetLineMode;
    Symbol symbols[64][128]; // array of symbols

    // data for alternavive renderer
    int cursorRow;
    int cursorPos;
    int frameCount;
    bool cursorBlinking;
    bool cursorUnderline;
    //CursorFormat cursorFormat;

    // data for format change check
    int nHrChars;
    int nVrRows;
};



class Crt8275Raster;


class Crt8275 : public AddressableDevice, public IActive
{

    enum FetchStatus {
        FS_NO_FETCH,    // no fetch needed
        FS_BURST_SPACE, // pause between bursts
        FS_BEGIN_BURST, // first fetch in burst
        FS_BURST        // fetch
    };

    enum DisplayState {
        DS_NORMAL,      // normal
        DS_VSPTOEOR,    // VSP to the end of row is active
        DS_VSPTOEOF     // VSP to the end of frame is active
    };

    enum CrtCommand
    {
        CC_LOADCURSOR,
        CC_READLPEN,
        CC_RESET
    };

    public:
        Crt8275();
        virtual ~Crt8275();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        void setFrequency(int64_t freq) override;
        void init() override;
        void reset() override;
        std::string getDebugInfo() override;

        // derived from AddressableDevice
        void writeByte(int nAddr, uint8_t value) override;
        uint8_t readByte(int nAddr) override;

        // derived from ActiveDevice
        void operate() override;

        // Crt8275 own methods
        void attachCore(PlatformCore* core);
        void attachDMA(Dma8257* dma, int channel);

        const Frame* getFrame() {return &m_frame;}
        bool getRasterStarted() {return m_isRasterStarted;}
        bool getVsyncOccured();
        int getNRows() {return m_nRows;}
        int getNLines() {return m_nLines;}
        int getNCharsPerRow() {return m_nCharsPerRow;}
        int getHrChars() {return m_nHrChars;}
        int getVrRows() {return m_nVrRows;}
        bool getRasterPresent() {return m_isRasterStarted;}
        double getFrameRate();

        void setLpenPosition(int x, int y);

        static EmuObject* create(const EmuValuesList&) {return new Crt8275();}

        friend Crt8275Raster;

    private:
        Dma8257* m_dma;              // Linked DMA Controller
        int m_dmaChannel;            // DMA channel
        PlatformCore* m_core;        // Linked platform core
        Crt8275Raster* m_raster;     // Assosiated raster device
        int m_cpuKDiv;               // CPU div factor

        int  m_nRows;                // row count
        int  m_nLines;               // line count
        bool m_isSpacedRows;         // spaced row count
        int  m_nCharsPerRow;         // characters per row
        int  m_undLine;              // undderline placement
        bool m_isOffsetLine;         // offset line couner mode
        bool m_isTransparentAttr;    // transparent attribute mode
        int  m_nVrRows;              // vertical retrace rows
        int  m_nHrChars;             // horisontal retrace characters
        int  m_burstCount;           // DMA cycles per burst
        int  m_burstSpaceCount;      // burst space count
        int  m_cursorPos;            // cursor char position
        int  m_cursorRow;            // cursor row position
        bool m_isIntsEnabled;        // enable interrupts flag
        bool m_cursorBlinking;
        bool m_cursorUnderline;
        int m_lpenX;                 // light pen X position
        int m_lpenY;                 // light pen Y position
        int m_lpenCorrection = 0;    // light pen position correction
        //CursorFormat m_cursorFormat; // cursor format

        uint8_t m_statusReg;
        uint8_t m_cmdReg;
        uint8_t m_resetParam[4];

        uint8_t m_rowBuf[80];        // char buffer
        uint8_t m_fifo[16];          // FIFO buffer

        Frame m_frame;               // output frame

        CrtCommand m_crtCmd;         // current CRT command
        int m_parameterNum;
        bool m_isCompleteCommand;

        bool m_isDisplayStarted;     // display started Flag

        //bool m_isDmaStarted;         // true if DMA not stopped due special code for the rest of the frame
        bool m_isRasterStarted;
        int m_curRow;                // current row
        int m_curBufPos;                // current position in buffer
        bool m_isNextCharToFifo;
        int m_curFifoPos;            // current FIFO position
        int m_curBurstPos;

        //bool m_needDmaData;
        bool m_isBurstSpace;
        bool m_isBurst;
        bool m_isDmaStoppedForRow;
        bool m_isDmaStoppedForFrame;
        //bool m_isDmaUnderrun;
        bool m_needExtraByte = false;
        bool m_wasVsync;

        bool m_wasDmaUnderrun;
        //int m_dmaUnderrunAtRow;


        void putCharToBuffer(uint8_t byte);
        void dmaUnderrun();
        void nextRow();
        void nextFrame();
//        void m_startDisplay();
        void stopDisplay();
        void presetCounters();
        void startRasterIfNotStarted();

        // Buffer Display Related Fields

        // [CCCC][Above Underline / Below Underline]
        const bool m_cCharAttrVsp[12][2] = {
            {1,0}, {1,0}, {0,1}, {0,1}, {1,0}, {0,0}, {0,0}, {0,1}, {1,1}, {0,0}, {0,0}, {0,0},
        };

        // [CCCC] (underline)
        const bool m_cCharAttrLten[12] = {0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0};

        bool m_curUnderline = false;
        bool m_curReverse = false;
        bool m_curBlink = false;
        bool m_curHighlight = false;
        bool m_curGpa1 = false;
        bool m_curGpa0 = false;
        int m_frameCount = 0;
        bool m_isBlankedToTheEndOfScreen = false;

        void prepareFrame();
        void displayBuffer();

};

class Crt8275Raster : public ActiveDevice
{
    public:
        Crt8275Raster();

        // derived from ActiveDevice
        void operate() override;

        // Crt8275 own methods

        friend Crt8275;

    private:
        Crt8275* m_crt;              // Master CRT device
        PlatformCore* m_core;        // Linked platform core
        //bool _isStarted;
        bool m_isHrtcActive = false;
        bool m_isVrtcActive = false;
        int m_curScanRow = 0;
        int m_curScanLine = 0;

        void startRaster();
        void stopRaster();
};

#endif // CRT75_H
