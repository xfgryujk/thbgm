#include "stdafx.h"
#include "libthbgm.h"
#include <Shlwapi.h>
#include <map>
#include <fstream>
#include <regex>
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
		if (fmtStream.gcount() != sizeof(fmtBuffer))
		{
			m_bgms.clear();
			return false;
		}
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

	bool THPosBgm::Save(const std::wstring& outputDir)
	{
		return true;
	}


	// THFmtBgm

	THFmtBgm::THFmtBgm(const std::wstring& fmtFile, const std::wstring& bgmFile, const std::wstring& cmtFile)
	{
		Load(fmtFile, bgmFile, cmtFile);
	}

	bool THFmtBgm::Load(const std::wstring& fmtFile, const std::wstring& bgmFile, const std::wstring& cmtFile)
	{
		return true;
	}

	bool THFmtBgm::Save(const std::wstring& outputDir)
	{
		return true;
	}
}
