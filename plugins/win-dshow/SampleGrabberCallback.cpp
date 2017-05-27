#include "SampleGrabberCallback.h"
#include "TCHAR.h"
#include "ImageFormatConversion.h"

SampleGrabberCallback::SampleGrabberCallback()
{
	m_bGetPicture = FALSE;
	ZeroMemory(m_szFileName, (MAX_PATH)*sizeof(wchar_t));

	//GetTempPath(MAX_PATH, m_szTempPath);
	//StringCchCat(m_szTempPath, MAX_PATH, TEXT("CaptureBmp"));
	//CreateDirectory(m_szTempPath, NULL);
}

ULONG STDMETHODCALLTYPE SampleGrabberCallback::AddRef()
{
	return 1;
}

ULONG STDMETHODCALLTYPE SampleGrabberCallback::Release()
{
	return 2;
}

HRESULT STDMETHODCALLTYPE SampleGrabberCallback::QueryInterface(REFIID riid,void** ppvObject)
{
	if (NULL == ppvObject) return E_POINTER;
	if (riid == __uuidof(IUnknown))
	{
		*ppvObject = static_cast<IUnknown*>(this);
		return S_OK;
	}
	if (riid == IID_ISampleGrabberCB)
	{
		*ppvObject = static_cast<ISampleGrabberCB*>(this);
		return S_OK;
	}
	return E_NOTIMPL;

}

HRESULT STDMETHODCALLTYPE SampleGrabberCallback::SampleCB(double Time, IMediaSample *pSample)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE SampleGrabberCallback::BufferCB(double Time, BYTE *pBuffer, long BufferLen)
{
	if(FALSE == m_bGetPicture)  // �ж��Ƿ���Ҫ��ͼ
		return S_FALSE;
	if(!pBuffer)
		return E_POINTER;

	SaveBitmap(pBuffer,BufferLen);

	m_bGetPicture = FALSE;
	return S_OK;
}

BOOL SampleGrabberCallback::SaveBitmap(BYTE * pBuffer, long lBufferSize)
{
	size_t len = _tcslen(m_szFileName);
	if (len == 0) {
		return FALSE;
	}
	wchar_t szBmpFileName[MAX_PATH];
	char szFileName[MAX_PATH];
	memset(szBmpFileName, 0, MAX_PATH);
	
	memcpy(szBmpFileName, m_szFileName, MAX_PATH);
	memcpy(szBmpFileName + (len - 3), L"bmp", 6);
	DWORD num = WideCharToMultiByte(CP_ACP, 0, szBmpFileName, _tcslen(szBmpFileName), NULL, 0, NULL, 0);
	memset(szFileName, 0, sizeof(szFileName));
	WideCharToMultiByte(CP_ACP, 0, szBmpFileName, _tcslen(szBmpFileName), szFileName, num, NULL, 0);
	szFileName[num] = '\0';

	HANDLE hf = CreateFileA(szFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);
	if(hf == INVALID_HANDLE_VALUE) {
		return E_FAIL;
	}

	BITMAPFILEHEADER bfh;
	ZeroMemory(&bfh,sizeof(bfh));
	bfh.bfType = 'MB';
	bfh.bfSize = sizeof(bfh) + lBufferSize + sizeof(BITMAPFILEHEADER);
	bfh.bfOffBits = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPFILEHEADER);
	
	DWORD dwWritten = 0;
	WriteFile( hf, &bfh, sizeof( bfh ), &dwWritten, NULL );    
	
	BITMAPINFOHEADER bih;
	ZeroMemory(&bih,sizeof(bih));
	bih.biSize = sizeof( bih );
	bih.biWidth = m_lWidth;
	bih.biHeight = m_lHeight;
	bih.biPlanes = 1;
	bih.biBitCount = m_iBitCount;  // Specifies the number of bits per pixel (bpp)
	WriteFile( hf, &bih, sizeof( bih ), &dwWritten, NULL );
	WriteFile( hf, pBuffer, lBufferSize, &dwWritten, NULL );
	CloseHandle( hf );

	// ����jpgͼƬ
	char szSrcFileName[MAX_PATH];
	char szDstFileName[MAX_PATH];
	memset(szSrcFileName, 0, sizeof(char)*(MAX_PATH));
	memset(szDstFileName, 0, sizeof(char)*(MAX_PATH));

	len = strlen(szFileName);
	memcpy(szSrcFileName, szFileName, len);
	memcpy(szDstFileName, szFileName, len);
	memcpy(szDstFileName + len - 3, "jpg", 3);
	CImageFormatConversion	ifc;
	ifc.ToJpg(szSrcFileName, szDstFileName, 100);
	::DeleteFileA(szSrcFileName);

	return TRUE;
}

BOOL SampleGrabberCallback::SetSavePicture(BOOL isSave)
{
	m_bGetPicture = isSave;
	return TRUE;
}

BOOL SampleGrabberCallback::SetPictureWH(long width, long height)
{
	m_lWidth = width; m_lHeight = height;
	return TRUE;
}

BOOL SampleGrabberCallback::SetPictureBitCount(int BitCount)
{
	m_iBitCount = BitCount;
	return TRUE;
}

BOOL SampleGrabberCallback::SetPictureFileName(wchar_t* filename)
{
	if (filename) {
		ZeroMemory(m_szFileName, (MAX_PATH) * sizeof(wchar_t));
		_tcscpy_s(m_szFileName, filename);
		//CopyMemory(m_szFileName, filename, _tcsclen(filename));
	}
	return TRUE;
}
