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
	BOOL SaveBitmap(BYTE * pBuffer, long lBufferSize ); // ±£´æbitmapÍ¼Æ¬
	BOOL SetSavePicture(BOOL isSave);
	BOOL SetPictureWH(long width, long height);
	BOOL SetPictureBitCount(int BitCount);
	BOOL SetPictureFileName(wchar_t* filename);

private:
	BOOL m_bGetPicture;  // is get a picture
	long m_lWidth;       // ´æ´¢Í¼Æ¬µÄ¿í¶È
	long m_lHeight;		 // ´æ´¢Í¼Æ¬µÄ³¤¶È
	int  m_iBitCount;    // the number of bits per pixel (bpp)
	wchar_t m_szFileName[MAX_PATH];
	//TCHAR m_szTempPath[MAX_PATH];
};

#endif //__SAMPLEGRABBERCALLBACK_H__