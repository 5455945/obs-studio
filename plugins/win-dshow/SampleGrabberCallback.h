#ifndef __SAMPLEGRABBERCALLBACK_H__
#define __SAMPLEGRABBERCALLBACK_H__
#include "common.h"

class SampleGrabberCallback : public ISampleGrabberCB
{
public:
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void** ppvObject);

	HRESULT STDMETHODCALLTYPE SampleCB(double Time, IMediaSample *pSample);
	HRESULT STDMETHODCALLTYPE BufferCB(double Time, BYTE *pBuffer, long BufferLen);
	
	SampleGrabberCallback();
	BOOL SaveBitmap(BYTE * pBuffer, long lBufferSize ); // ����bitmapͼƬ
	BOOL SetSavePicture(BOOL isSave);
	BOOL SetPictureWH(long width, long height);
	BOOL SetPictureBitCount(int BitCount);
	BOOL SetPictureFileName(wchar_t* filename);

private:
	BOOL m_bGetPicture;  // is get a picture
	long m_lWidth;       // �洢ͼƬ�Ŀ��
	long m_lHeight;		 // �洢ͼƬ�ĳ���
	int  m_iBitCount;    // the number of bits per pixel (bpp)
	wchar_t m_szFileName[MAX_PATH];
	//TCHAR m_szTempPath[MAX_PATH];
};

#endif //__SAMPLEGRABBERCALLBACK_H__