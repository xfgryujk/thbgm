#include "stdafx.h"
#include "libthbgm.h"
#include <Shlwapi.h>
#include <map>
#include <fstream>
#include <regex>
#include <bass.h>
#include <bassmix.h>
using namespace std;


namespace thbgm
{
	namespace
	{
		wstring StringToWstring(const string& src, UINT codePage = CP_THREAD_ACP)
		{
			int dstLen = MultiByteToWideChar(codePage, 0, src.c_str(), src.size(), NULL, 0);
			if (dstLen == 0)
				return L"";
			wstring res((size_t)dstLen, L'\0');
			MultiByteToWideChar(codePage, 0, src.c_str(), src.size(), (LPWSTR)res.c_str(), dstLen);
			res.resize(dstLen);
			return res;
		}

		string WstringToString(const wstring& src, UINT codePage = CP_THREAD_ACP)
		{
			int dstLen = WideCharToMultiByte(codePage, 0, src.c_str(), src.size(), NULL, 0, NULL, NULL);
			if (dstLen == 0)
				return "";
			string res((size_t)dstLen, '\0');
			WideCharToMultiByte(codePage, 0, src.c_str(), src.size(), (LPSTR)res.c_str(), dstLen, NULL, NULL);
			res.resize(dstLen);
			return res;
		}


		UINT GuessCmtCodePage(const string& src)
		{
			// feature (keyword) -> code page
			static map<string, UINT> s_knownCodePages{
				{ "\x82\x53\x82\x54", 932 },         // Japanese (Shift-JIS)
				{ "\xA3\xB4\xA3\xB5", 936 },         // Chinese Simplified (GBK)
				{ "\xA2\xB3\xA2\xB4", 950 }          // Chinese Traditional (Big5)
			};
			for (const auto& i : s_knownCodePages)
			{
				if (src.find(i.first) != string::npos)
					return i.second;
			}
			return 932; // Shift-JIS for default
		}

		void GetDisplayNames(vector<Bgm>& bgms, const wstring& cmtFile)
		{
			ifstream cmtStream(cmtFile);
			if (!cmtStream.is_open())
			{
				for (auto& i : bgms)
					i.displayName = i.pureFileName;
			}
			else
			{
				filebuf* pbuf = cmtStream.rdbuf();
				size_t size = (size_t)pbuf->pubseekoff(0, cmtStream.end, cmtStream.in);
				pbuf->pubseekpos(0, cmtStream.in);
				string cmtBuffer(size, '\0');
				pbuf->sgetn((char*)cmtBuffer.c_str(), size);

				UINT cmtCodePage = GuessCmtCodePage(cmtBuffer);

				for (auto& i : bgms)
				{
					regex reg("^@bgm/" + WstringToString(i.pureFileName) + R"#(.*?\n(.*?)\r?$)#");
					smatch match;
					if (!regex_search(cmtBuffer, match, reg))
						i.displayName = i.pureFileName;
					else
						i.displayName = StringToWstring(match[1].str(), cmtCodePage);
				}
			}
		}


		bool WritePcm(Bgm& bgm, ofstream& output, DWORD frequency = 44100, DWORD channels = 2)
		{
			HSTREAM mixedStream = BASS_Mixer_StreamCreate(frequency, channels, BASS_STREAM_DECODE | BASS_MIXER_END);
			if (mixedStream == NULL) return false;
			HSTREAM srcStream = BASS_StreamCreateFile(FALSE, WstringToString(bgm.newFileName).c_str(), 0, 0, BASS_STREAM_DECODE);
			if (srcStream == NULL) { BASS_StreamFree(mixedStream); return false; }

			bool res = true;
			auto buffer = make_unique<BYTE[]>(10240);

			// Resampling
			if (!BASS_Mixer_StreamAddChannel(mixedStream, srcStream, BASS_MIXER_BUFFER)) { res = false; goto End; }
			bgm.newEndPoint = 0;
			while (BASS_ChannelIsActive(mixedStream) != BASS_ACTIVE_STOPPED)
			{
				DWORD size = BASS_ChannelGetData(mixedStream, buffer.get(), 10240);
				if (size == -1) { res = false; goto End; }
				output.write((char*)buffer.get(), size);
				bgm.newEndPoint += size;
			}

		End:
			BASS_StreamFree(srcStream);
			BASS_StreamFree(mixedStream);
			return res;
		}

		void CopyStream(istream& input, ostream& output, size_t size)
		{
			auto buffer = make_unique<BYTE[]>(10240);
			while (size > 10240)
			{
				input.read((char*)buffer.get(), 10240);
				output.write((char*)buffer.get(), 10240);
				size -= 10240;
			}
			input.read((char*)buffer.get(), size);
			output.write((char*)buffer.get(), size);
		}
	}


	bool Init(HWND hwnd)
	{
		/*if (HIWORD(BASS_GetVersion()) != BASSVERSION)
			return false;*/
		return BASS_Init(0, 44100, 0, hwnd, NULL) != FALSE;
	}

	bool Uninit()
	{
		return BASS_Free() != FALSE;
	}


	unique_ptr<THBgm> THBgm::Create(const wstring& fmtFile, const wstring& bgmFile, const wstring& cmtFile)
	{
		if (fmtFile.size() < 4)
			return nullptr;
		wstring ext = fmtFile.substr(fmtFile.size() - 4, 4);

		if (_wcsicmp(ext.c_str(), L".pos") == 0)
			return make_unique<THPosBgm>(fmtFile, bgmFile, cmtFile);
		if (_wcsicmp(ext.c_str(), L".fmt") == 0)
			return make_unique<THFmtBgm>(fmtFile, bgmFile, cmtFile);
		return nullptr;
	}


	// THPosBgm

	THPosBgm::THPosBgm(const wstring& fmtFile, const wstring& bgmFile, const wstring& cmtFile)
	{
		Load(fmtFile, bgmFile, cmtFile);
	}

	bool THPosBgm::Load(const wstring& fmtFile, const wstring& bgmFile, const wstring& cmtFile)
	{
		m_fmtFile = fmtFile;
		m_bgmFile = bgmFile;
		m_bgms.clear();

		if (!PathFileExistsW(fmtFile.c_str()) || !PathFileExistsW(bgmFile.c_str()))
			return false;
		ifstream fmtStream(fmtFile, ios_base::binary);
		if (!fmtStream.is_open())
			return false;

		m_bgms.resize(1);
		Bgm& bgm = m_bgms[0];

		// fileName
		size_t pos = bgmFile.rfind(L'\\');
		if (pos != wstring::npos)
			bgm.fileName = bgmFile.substr(pos + 1, bgmFile.size() - pos - 1);
		else
			bgm.fileName = bgmFile;

		// pureFileName
		pos = bgm.fileName.rfind(L'.');
		if (pos != wstring::npos)
			bgm.pureFileName = bgm.fileName.substr(0, pos);
		else
			bgm.pureFileName = bgm.fileName;

		// BGM information
		DWORD fmtBuffer[2];
		fmtStream.read((char*)fmtBuffer, sizeof(fmtBuffer));
		bgm.originalOffset = 44;
		bgm.originalLoopPoint = fmtBuffer[0] * 4 - bgm.originalOffset;
		bgm.originalEndPoint = fmtBuffer[1] * 4 - bgm.originalOffset;
		bgm.newOffset = bgm.originalOffset;
		bgm.newLoopPoint = bgm.originalLoopPoint;
		bgm.newEndPoint = bgm.originalEndPoint;

		// displayName
		GetDisplayNames(m_bgms, cmtFile);

		return true;
	}

	bool THPosBgm::Save(const wstring& outputDir)
	{
		Bgm& bgm = m_bgms[0];

		bgm.newOffset = 44;

		// Write BGM
		if (bgm.newFileName.empty())
		{
			if (!CopyFileW(m_bgmFile.c_str(), (outputDir + L"\\" + m_bgms[0].fileName).c_str(), FALSE))
				return false;

			bgm.newEndPoint = bgm.originalEndPoint;
		}
		else
		{
			ofstream newBgmStream(outputDir + L"\\" + m_bgms[0].fileName, ios_base::binary);
			if (!newBgmStream.is_open())
				return false;

			// Write WAV header
			newBgmStream.write("RIFF\0\0\0\0WAVEfmt \20\0\0\0", 20);
			WAVEFORMATEX wf;
			wf.wFormatTag = 1;
			wf.nChannels = 2;
			wf.wBitsPerSample = 16;
			wf.nBlockAlign = wf.nChannels * wf.wBitsPerSample / 8;
			wf.nSamplesPerSec = 44100;
			wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
			newBgmStream.write((char*)&wf, 16);
			newBgmStream.write("data\0\0\0\0", 8);

			// Write data
			if (!WritePcm(bgm, newBgmStream))
				return false;

			// Complete WAV header
			DWORD buffer;
			newBgmStream.seekp(4, ios::beg);
			buffer = bgm.newOffset + bgm.newEndPoint - 4;
			newBgmStream.write((char*)&buffer, 4);
			newBgmStream.seekp(40, ios::beg);
			buffer = bgm.newEndPoint;
			newBgmStream.write((char*)&buffer, 4);
		}
		
		// Write FMT
		DWORD fmtBuffer[2] = {
			(bgm.newLoopPoint + bgm.newOffset) / 4,
			(bgm.newEndPoint + bgm.newOffset) / 4
		};
		ofstream newFmtStream(outputDir + L"\\" + m_bgms[0].pureFileName + L".pos", ios_base::binary);
		if (!newFmtStream.is_open())
			return false;
		newFmtStream.write((char*)fmtBuffer, sizeof(fmtBuffer));

		return true;
	}


	// THFmtBgm

	THFmtBgm::THFmtBgm(const wstring& fmtFile, const wstring& bgmFile, const wstring& cmtFile)
	{
		Load(fmtFile, bgmFile, cmtFile);
	}

	bool THFmtBgm::Load(const wstring& fmtFile, const wstring& bgmFile, const wstring& cmtFile)
	{
		m_fmtFile = fmtFile;
		m_bgmFile = bgmFile;
		m_bgms.clear();

		if (!PathFileExistsW(fmtFile.c_str()) || !PathFileExistsW(bgmFile.c_str()))
			return false;
		ifstream fmtStream(fmtFile, ios_base::binary);
		if (!fmtStream.is_open())
			return false;

		filebuf* pbuf = fmtStream.rdbuf();
		m_fmtSize = (size_t)pbuf->pubseekoff(0, fmtStream.end, fmtStream.in);
		pbuf->pubseekpos(0, fmtStream.in);
		m_fmtBuffer = make_unique<BYTE[]>(m_fmtSize);
		pbuf->sgetn((char*)m_fmtBuffer.get(), m_fmtSize);

		for (FmtStruct* pFmt = (FmtStruct*)m_fmtBuffer.get(); (BYTE*)pFmt + sizeof(FmtStruct) <= m_fmtBuffer.get() + m_fmtSize; ++pFmt)
		{
			m_bgms.resize(m_bgms.size() + 1);
			Bgm& bgm = m_bgms[m_bgms.size() - 1];

			// fileName
			bgm.fileName = StringToWstring(pFmt->fileName);

			// pureFileName
			size_t pos = bgm.fileName.rfind(L'.');
			if (pos != wstring::npos)
				bgm.pureFileName = bgm.fileName.substr(0, pos);
			else
				bgm.pureFileName = bgm.fileName;

			// BGM information
			bgm.originalOffset = pFmt->offset;
			bgm.originalLoopPoint = pFmt->loopPoint;
			bgm.originalEndPoint = pFmt->endPoint;
			bgm.newOffset = bgm.originalOffset;
			bgm.newLoopPoint = bgm.originalLoopPoint;
			bgm.newEndPoint = bgm.originalEndPoint;
		}

		// displayName
		GetDisplayNames(m_bgms, cmtFile);

		return true;
	}

	bool THFmtBgm::Save(const wstring& outputDir)
	{
		// Write BGM
		ifstream originalBgmStream(m_bgmFile.c_str(), ios_base::binary);
		ofstream newBgmStream(outputDir + L"\\thbgm.dat", ios_base::binary);
		if (!newBgmStream.is_open() || !newBgmStream.is_open())
			return false;

		CopyStream(originalBgmStream, newBgmStream, 16);
		DWORD offset = 16;

		for (auto& i : m_bgms)
		{
			i.newOffset = offset;
			if (i.newFileName.empty())
			{
				originalBgmStream.seekg(i.originalOffset, ios::beg);
				i.newEndPoint = i.originalEndPoint;
				CopyStream(originalBgmStream, newBgmStream, i.newEndPoint);
			}
			else
			{
				if (!WritePcm(i, newBgmStream))
					return false;
			}
			offset += i.newEndPoint;
		}

		// Write FMT
		ofstream newFmtStream(outputDir + L"\\thbgm.fmt", ios_base::binary);
		if (!newFmtStream.is_open())
			return false;

		FmtStruct* pFmt = (FmtStruct*)m_fmtBuffer.get();
		for (const auto& i : m_bgms)
		{
			pFmt->offset = i.newOffset;
			pFmt->loopPoint = i.newLoopPoint - i.newLoopPoint % 4; // Make sure it is divisible by 4
			pFmt->endPoint = i.newEndPoint;

			++pFmt;
		}

		newFmtStream.write((char*)m_fmtBuffer.get(), m_fmtSize);

		return true;
	}
}
