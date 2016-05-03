/*
 * oglosd.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include "oglosd.h"
#include "openglosd.h"

//////////////////////////////////////////////////////////////////////////////
//	OSD provider
//////////////////////////////////////////////////////////////////////////////
cOsd *cOglOsdProvider::Osd;		///< single osd

std::shared_ptr<cOglThread> oglThread;    ///< openGL worker Thread

int cOglOsdProvider::StoreImageData(const cImage &Image)
{
    if (StartOpenGlThread()) {
        int imgHandle = oglThread->StoreImage(Image);
        return imgHandle;
    }
    return 0;
}

void cOglOsdProvider::DropImageData(int ImageHandle)
{
    if (StartOpenGlThread())
        oglThread->DropImageData(ImageHandle);
}

/**
**	Create a new OSD.
**
**	@param left	x-coordinate of OSD
**	@param top	y-coordinate of OSD
**	@param level	layer level of OSD
*/
cOsd *cOglOsdProvider::CreateOsd(int left, int top, uint level)
{
    dsyslog("[oglosd]%s: %d, %d, %d, using OpenGL OSD support\n", __FUNCTION__, left, top, level);
    if (StartOpenGlThread())
        return Osd = new cOglOsd(left, top, level, oglThread);
    //return dummy osd if shd is detached
    return NULL;
}

/**
**	Check if this OSD provider is able to handle a true color OSD.
**
**	@returns true we are able to handle a true color OSD.
*/
bool cOglOsdProvider::ProvidesTrueColor(void)
{
    return true;
}

const cImage *cOglOsdProvider::GetImageData(int ImageHandle) {
    return cOsdProvider::GetImageData(ImageHandle);
}

void cOglOsdProvider::OsdSizeChanged(void) {
    //cleanup OpenGl Context
    cOglOsdProvider::StopOpenGlThread();
    cOsdProvider::UpdateOsdSize();
}


bool cOglOsdProvider::StartOpenGlThread(void) {
    //only try to start worker thread if shd is attached
    //otherwise glutInit() crashes
	if (pVMed->IsDeviceSuspended()) {
        return false;
    }
    if (oglThread.get()) {
        if (oglThread->Active()) {
            return true;
        }
        oglThread.reset();
    }
    cCondWait wait;
    dsyslog("[oglosd]Trying to start OpenGL Worker Thread");
	oglThread.reset(new cOglThread(&wait, pVMed->MaxSizeGPUImageCache()));
    wait.Wait();
    if (oglThread->Active()) {
        dsyslog("[oglosd]OpenGL Worker Thread successfully started");
        return true;
    }
    dsyslog("[oglosd]openGL Thread NOT successfully started");
    return false;
}

void cOglOsdProvider::StopOpenGlThread(void) {
    dsyslog("[oglosd]stopping OpenGL Worker Thread ");
    if (oglThread) {
        oglThread->Stop();
    }
    oglThread.reset();
    dsyslog("[oglosd]OpenGL Worker Thread stopped");
}

/**
**	Create cOsdProvider class.
*/
cOglOsdProvider::cOglOsdProvider(void)
:  cOsdProvider()
{
#ifdef OSD_DEBUG
    dsyslog("[oglosd]%s:\n", __FUNCTION__);
#endif
    StopOpenGlThread();
    ////////////// LUCIAN VideoSetVideoEventCallback(&OsdSizeChanged);
}

/**
**	Destroy cOsdProvider class.
*/
cOglOsdProvider::~cOglOsdProvider()
{
#ifdef OSD_DEBUG
    dsyslog("[oglosd]%s:\n", __FUNCTION__);
#endif
    StopOpenGlThread();
}
