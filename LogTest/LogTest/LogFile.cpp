#include "stdafx.h"
#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include "LogFile.h"


CLogFile::CLogFile(long lTruncate /*= 4096*/)
{
	m_pLogFile = nullptr;
	m_lTruncate = lTruncate;
	m_wCount = 0U;

	CreateLogsDir();
}

CLogFile::~CLogFile()
{
	CloseFile();
}

void CLogFile::CreateLogsDir()
{
	TCHAR szFile[MAX_PATH];

	if (::GetModuleFileName(nullptr, szFile, MAX_PATH) == 0 || GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		PrintError(_T("GetModuleFileName"));
	else {
		tstring::size_type pos = tstring(szFile).find_last_of(_T("\\/"));
		tstring path = tstring(szFile).substr(0, pos) + _T("\\logs");
		if (CreateDirectory(path.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS)
			ProcessName(path);
		else
			PrintError(_T("CreateDirectory"));
	}
}

void CLogFile::ProcessName(tstring strPath)
{
	FILETIME lastDate = { 0, 0 };
	FILETIME curDate;
	tstring filename;
	CFileFind finder;

	if (_tchdir(strPath.c_str()) != 0)
		_tprintf(_T("Failed to change directory.\n"));
	else {
		tstring filemask = _T("*.log");

		BOOL bWorking = finder.FindFile(filemask.c_str());

		while (bWorking) {
			bWorking = finder.FindNextFile();
			finder.GetCreationTime(&curDate);
			if (CompareFileTime(&curDate, &lastDate) > 0) {
				lastDate = curDate;
				filename = finder.GetFileName().GetString();
			}
		}

		finder.Close();

		if (filename.empty())
			filename = FormatName();

		OpenFile(filename.c_str());
	}
}

tstring CLogFile::FormatName()
{
	SYSTEMTIME systime;
	CString str;

	GetLocalTime(&systime);

	str.Format(_T("%04d%02d%02d_%02d%02d%02d_%04x.log"),
		systime.wYear, systime.wMonth, systime.wDay, systime.wHour, systime.wMinute, systime.wSecond, m_wCount);
	
	m_wCount++;

	return tstring(str.GetString());
}

void CLogFile::OpenFile(LPCTSTR pszFile)
{
	if ((m_pLogFile = _tfsopen(pszFile, _T("at"), _SH_DENYWR)) == nullptr)
		_tprintf(_T("Error opening log file %s.\n"), pszFile);
}

void CLogFile::CloseFile()
{
	if (m_pLogFile) {
		fclose(m_pLogFile);
		m_pLogFile = nullptr;
	}
}

void CLogFile::WriteFile(LPCTSTR pszFormat, ...)
{
	if (!m_pLogFile)
		return;

	TCHAR szBuffer[1024];
	va_list args;
	va_start(args, pszFormat);
	_vsntprintf_s(szBuffer, _countof(szBuffer), _TRUNCATE, pszFormat, args);
	va_end(args);

	if (fseek(m_pLogFile, 0, SEEK_END) == 0) {
		long lLength = ftell(m_pLogFile);

		if (lLength >= m_lTruncate) {
			CloseFile();
			OpenFile(FormatName().c_str());
		}

		if (m_pLogFile) {
			SYSTEMTIME	time;
			::GetLocalTime(&time);

			_ftprintf(m_pLogFile, _T("-----[START]: %04d/%02d/%02d %02d:%02d:%02d.%03d\n%s\n-----[END]\n\n"),
				time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds, szBuffer);

			fflush(m_pLogFile);
		}
	}
}

void CLogFile::PrintError(TCHAR *pMsg)
{
	LPTSTR pBuffer = nullptr;
	DWORD dwErr = GetLastError();

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS, 
		nullptr, dwErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&pBuffer, 0, nullptr);

	if (pBuffer) {
#ifdef _UNICODE
		_setmode(_fileno(stdout), _O_U16TEXT);
#endif
		_tprintf(_T("%s failed with error 0x%X: %s"), pMsg, dwErr, pBuffer);
		LocalFree(pBuffer);
		pBuffer = nullptr;
	}
}
