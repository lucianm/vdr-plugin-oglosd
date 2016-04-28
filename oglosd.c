/*
 * oglosd.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>


#include "openglosd.h"


static const char *VERSION        = "0.0.1";
static const char *DESCRIPTION    = "Generic OpenGL/ES OSD provider";
static const char *MAINMENUENTRY  = "OpenGL OSD";

static int ConfigMaxSizeGPUImageCache = 128;  ///< maximum size of GPU mem to be used for image caching

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
    static std::shared_ptr<cOglThread> oglThread;
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

cOsd *cOglOsdProvider::Osd;		///< single osd

std::shared_ptr<cOglThread> cOglOsdProvider::oglThread;    ///< openGL worker Thread

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
    if (SuspendMode != NOT_SUSPENDED) {
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
    oglThread.reset(new cOglThread(&wait, ConfigMaxSizeGPUImageCache));
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
    VideoSetVideoEventCallback(&OsdSizeChanged);
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







//////////////////////////////////////////////////////////////////////////////
//	cMenuSetupPage
//////////////////////////////////////////////////////////////////////////////

/**
**	Soft device plugin menu setup page class.
*/
class cMenuSetupOglOsd:public cMenuSetupPage
{
  protected:
    ///
    /// local copies of global setup variables:
    /// @{
    int MaxSizeGPUImageCache;
    /// @}
  private:
    void Create(void);			// create sub-menu
  protected:
    virtual void Store(void);
  public:
    cMenuSetupOglOsd(void);
    virtual eOSState ProcessKey(eKeys);	// handle input
};

/**
**	Create setup menu.
*/
void cMenuSetupOglOsd::Create(void)
{
    int current;
    current = Current();		// get current menu item index
    Clear();				// clear the menu

	//
	//	osd
	//
    Add(new cMenuEditIntItem(tr("GPU mem used for image caching (MB)"), &MaxSizeGPUImageCache, 0, 4000));
    SetCurrent(Get(current));		// restore selected menu entry
    Display();				// display build menu
}

/**
**	Constructor setup menu.
**
**	Import global config variables into setup.
*/
cMenuSetupOglOsd::cMenuSetupOglOsd(void)
{
    MaxSizeGPUImageCache = ConfigMaxSizeGPUImageCache;
    Create();
}

/**
**	Store setup.
*/
void cMenuSetupOglOsd::Store(void)
{
    SetupStore("MaxSizeGPUImageCache", ConfigMaxSizeGPUImageCache = MaxSizeGPUImageCache);
}






class cPluginOglOsd : public cPlugin {
private:
  // Add any member variables or functions you may need here.
public:
  cPluginOglOsd(void);
  virtual ~cPluginOglOsd();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return DESCRIPTION; }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual void MainThreadHook(void);
  virtual cString Active(void);
  virtual time_t WakeupTime(void);
  virtual const char *MainMenuEntry(void) { return MAINMENUENTRY; }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
  };

cPluginOglOsd::cPluginOglOsd(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginOglOsd::~cPluginOglOsd()
{
  // Clean up after yourself!
}

const char *cPluginOglOsd::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginOglOsd::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginOglOsd::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  return true;
}

bool cPluginOglOsd::Start(void)
{
  // Start any background activities the plugin shall perform.
  return true;
}

void cPluginOglOsd::Stop(void)
{
  // Stop any background activities the plugin is performing.
}

void cPluginOglOsd::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

void cPluginOglOsd::MainThreadHook(void)
{
  // Perform actions in the context of the main program thread.
  // WARNING: Use with great care - see PLUGINS.html!
}

cString cPluginOglOsd::Active(void)
{
  // Return a message string if shutdown should be postponed
  return NULL;
}

time_t cPluginOglOsd::WakeupTime(void)
{
  // Return custom wakeup time for shutdown script
  return 0;
}

cOsdObject *cPluginOglOsd::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
  return NULL;
}

cMenuSetupPage *cPluginOglOsd::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return new cMenuSetupOglOsd;
}

bool cPluginOglOsd::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
    if (!strcasecmp(Name, "MaxSizeGPUImageCache")) {
      ConfigMaxSizeGPUImageCache = atoi(Value);
      return true;
	}
  return false;
}

bool cPluginOglOsd::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  return false;
}

const char **cPluginOglOsd::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  return NULL;
}

cString cPluginOglOsd::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  return NULL;
}

VDRPLUGINCREATOR(cPluginOglOsd); // Don't touch this!
