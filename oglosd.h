/*
 * psl-oglosd.h: A Plugin Shared Library for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#ifndef _OGLOSD_INCLUDED
#define _OGLOSD_INCLUDED

#define PSL_OGLOSD_VERSION "0.0.1";
#define PSL_OGLOSD_VERSNUM 00001  // Version * 10000 + Major * 100 + Minor


#include <vdr/osd.h>

extern bool (*CbIsDeviceSuspended)(void);


//////////////////////////////////////////////////////////////////////////////
//	OSD provider
//////////////////////////////////////////////////////////////////////////////

/**
**	Soft device plugin OSD provider class.
*/
class cOglOsdProvider:public cOsdProvider
{
  private:
    static cOsd *Osd;			///< single OSD
//    static std::shared_ptr<cOglThread> oglThread;
    static bool StartOpenGlThread(void);
protected:
    virtual int StoreImageData(const cImage &Image);
    virtual void DropImageData(int ImageHandle);
  public:
    virtual cOsd * CreateOsd(int, int, uint);
    virtual bool ProvidesTrueColor(void);
    static void StopOpenGlThread(void);
    static const cImage *GetImageData(int ImageHandle);
    static void OsdSizeChanged(void);
    cOglOsdProvider(void);		///< OSD provider constructor
    virtual ~cOglOsdProvider();	///< OSD provider destructor
};

#endif
