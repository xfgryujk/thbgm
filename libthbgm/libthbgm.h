#pragma once
#include <string>
#include <memory>
#include <vector>


namespace thbgm
{
	bool Init(HWND hwnd = NULL);
	bool Uninit();

	struct Bgm
	{
		std::wstring fileName;         // File name in FMT file or BGM directory (TH06)
		std::wstring pureFileName;     // File name excluding extension
		std::wstring displayName;      // Music name in musiccmt.txt
		DWORD originalOffset;          // Offset in thbgm.dat or 44 in WAV file (TH06)
		DWORD originalLoopPoint;       // The loop start point, relative to offset
		DWORD originalEndPoint;        // Length of the music

		std::wstring newFileName;      // The file to replace with
		DWORD newOffset;               // Automatically calculated by Save()
		DWORD newLoopPoint;			   // Set by user
		DWORD newEndPoint;			   // Automatically calculated by Save()->WritePcm()
	};

	class THBgm
	{
	public:
		static std::unique_ptr<THBgm> Create(const std::wstring& fmtFile, const std::wstring& bgmFile, const std::wstring& cmtFile);

		virtual ~THBgm() = default;


		std::wstring m_fmtFile;
		std::wstring m_bgmFile;
		std::vector<Bgm> m_bgms;

		virtual bool Load(const std::wstring& fmtFile, const std::wstring& bgmFile, const std::wstring& cmtFile) = 0;
		virtual bool Save(const std::wstring& outputDir) = 0;
	};

	class THPosBgm : public THBgm
	{
	public:
		THPosBgm() = default;
		THPosBgm(const std::wstring& fmtFile, const std::wstring& bgmFile, const std::wstring& cmtFile);
		virtual ~THPosBgm() = default;

		virtual bool Load(const std::wstring& fmtFile, const std::wstring& bgmFile, const std::wstring& cmtFile);
		virtual bool Save(const std::wstring& outputDir);
	};

	class THFmtBgm : public THBgm
	{
	public:
		THFmtBgm() = default;
		THFmtBgm(const std::wstring& fmtFile, const std::wstring& bgmFile, const std::wstring& cmtFile);
		virtual ~THFmtBgm() = default;

		virtual bool Load(const std::wstring& fmtFile, const std::wstring& bgmFile, const std::wstring& cmtFile);
		virtual bool Save(const std::wstring& outputDir);

	protected:
		struct FmtStruct
		{
			char fileName[16];
			DWORD offset;
			DWORD unknown1;
			DWORD loopPoint;
			DWORD endPoint;
			DWORD unknown2;
			DWORD frequency;
			DWORD bytesPerSec;
			DWORD unknown3;
			DWORD zero;
		};
		static_assert(sizeof(FmtStruct) == 52, "Wrong size!");
	};
}
