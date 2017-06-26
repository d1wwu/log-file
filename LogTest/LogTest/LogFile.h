#pragma once

using namespace std;

typedef std::basic_string<TCHAR> tstring;


class CLogFile
{
	FILE *m_pLogFile;
	long m_lTruncate;
	WORD m_wCount;

	void CreateLogsDir();
	void ProcessName(tstring strPath);
	tstring FormatName();
	void OpenFile(LPCTSTR pszFile);
	void CloseFile();

public:

	CLogFile(long lTruncate = 4096);
	~CLogFile();
	
	void WriteFile(LPCTSTR pszFormat, ...);
	void PrintError(PTCHAR pMsg);
};
