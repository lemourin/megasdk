/**
 * @file megaapi_impl.h
 * @brief Private header file of the intermediate layer for the MEGA C++ SDK.
 *
 * (c) 2013-2014 by Mega Limited, Auckland, New Zealand
 *
 * This file is part of the MEGA SDK - Client Access Engine.
 *
 * Applications using the MEGA API must present a valid application key
 * and comply with the the rules set forth in the Terms of Service.
 *
 * The MEGA SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * @copyright Simplified (2-clause) BSD License.
 *
 * You should have received a copy of the license along with this
 * program.
 */

#ifndef MEGAAPI_IMPL_H
#define MEGAAPI_IMPL_H

#include "mega.h"
#include "mega/gfx/external.h"
#include "megaapi.h"

#define CRON_USE_LOCAL_TIME 1
#include "mega/mega_ccronexpr.h"

#ifdef USE_PCRE
#include <pcre.h>
#endif

#ifdef HAVE_LIBUV
#include "uv.h"
#include "mega/mega_http_parser.h"
#include "mega/mega_evt_tls.h"

#endif

#ifndef _WIN32
#include <curl/curl.h>
#include <fcntl.h>
#endif

////////////////////////////// SETTINGS //////////////////////////////
////////// Support for threads and mutexes
//Choose one of these options.
//Otherwise, C++11 threads and mutexes will be used
//#define USE_PTHREAD
//#define USE_QT

////////// Support for thumbnails and previews.
//If you selected QT for threads and mutexes, it will be also used for thumbnails and previews
//You can create a subclass of MegaGfxProcessor and pass it to the constructor of MegaApi
//#define USE_FREEIMAGE

//Define WINDOWS_PHONE if you want to build the MEGA SDK for Windows Phone
//#define WINDOWS_PHONE
/////////////////////////// END OF SETTINGS ///////////////////////////

namespace mega
{

#ifdef USE_QT
class MegaThread : public QtThread {};
class MegaMutex : public QtMutex
{
public:
    MegaMutex() : QtMutex() { }
    MegaMutex(bool recursive) : QtMutex(recursive) { }
};
class MegaSemaphore : public QtSemaphore {};
#elif USE_PTHREAD
class MegaThread : public PosixThread {};
class MegaMutex : public PosixMutex
{
public:
    MegaMutex() : PosixMutex() { }
    MegaMutex(bool recursive) : PosixMutex(recursive) { }
};
class MegaSemaphore : public PosixSemaphore {};
#elif defined(_WIN32) && !defined(USE_CPPTHREAD) && !defined(WINDOWS_PHONE)
class MegaThread : public Win32Thread {};
class MegaMutex : public Win32Mutex
{
public:
    MegaMutex() : Win32Mutex() { }
    MegaMutex(bool recursive) : Win32Mutex(recursive) { }
};
class MegaSemaphore : public Win32Semaphore {};
#else
class MegaThread : public CppThread {};
class MegaMutex : public CppMutex
{
public:
    MegaMutex() : CppMutex() { }
    MegaMutex(bool recursive) : CppMutex(recursive) { }
};
class MegaSemaphore : public CppSemaphore {};
#endif

#ifdef USE_QT
class MegaGfxProc : public GfxProcQT {};
#elif USE_FREEIMAGE
class MegaGfxProc : public GfxProcFreeImage {};
#elif TARGET_OS_IPHONE
class MegaGfxProc : public GfxProcCG {};
#else
class MegaGfxProc : public GfxProcExternal {};
#endif

#ifdef WIN32
    #ifndef WINDOWS_PHONE
    #ifdef USE_CURL
    class MegaHttpIO : public CurlHttpIO {};
    #else
    class MegaHttpIO : public WinHttpIO {};
    #endif
    #else
    class MegaHttpIO : public CurlHttpIO {};
    #endif
	class MegaFileSystemAccess : public WinFileSystemAccess {};
	class MegaWaiter : public WinWaiter {};
#else
    #ifdef __APPLE__
    typedef CurlHttpIO MegaHttpIO;
    typedef PosixFileSystemAccess MegaFileSystemAccess;
    typedef PosixWaiter MegaWaiter;
    #else
    class MegaHttpIO : public CurlHttpIO {};
    class MegaFileSystemAccess : public PosixFileSystemAccess {};
    class MegaWaiter : public PosixWaiter {};
    #endif
#endif

#ifdef HAVE_LIBUV
class MegaTCPServer;
class MegaHTTPServer;
class MegaFTPServer;
#endif

class MegaDbAccess : public SqliteDbAccess
{
	public:
		MegaDbAccess(string *basePath = NULL) : SqliteDbAccess(basePath){}
};

class ExternalLogger : public Logger
{
public:
    ExternalLogger();
    ~ExternalLogger();
    void addMegaLogger(MegaLogger* logger);
    void removeMegaLogger(MegaLogger *logger);
    void setLogLevel(int logLevel);
    void setLogToConsole(bool enable);
    void postLog(int logLevel, const char *message, const char *filename, int line);
    virtual void log(const char *time, int loglevel, const char *source, const char *message);

private:
    MegaMutex mutex;
    set <MegaLogger *> megaLoggers;
    bool logToConsole;
};

class MegaTransferPrivate;
class MegaTreeProcCopy : public MegaTreeProcessor
{
public:
    NewNode* nn;
    unsigned nc;

    MegaTreeProcCopy(MegaClient *client);
    virtual bool processMegaNode(MegaNode* node);
    void allocnodes(void);

protected:
    MegaClient *client;
};


class MegaSizeProcessor : public MegaTreeProcessor
{
    protected:
        long long totalBytes;

    public:
        MegaSizeProcessor();
        virtual bool processMegaNode(MegaNode* node);
        long long getTotalBytes();
};

class MegaFolderUploadController : public MegaRequestListener, public MegaTransferListener
{
public:
    MegaFolderUploadController(MegaApiImpl *megaApi, MegaTransferPrivate *transfer);
    void start();

protected:
    void onFolderAvailable(MegaHandle handle);
    void checkCompletion();

    std::list<std::string> pendingFolders;
    MegaApiImpl *megaApi;
    MegaClient *client;
    MegaTransferPrivate *transfer;
    MegaTransferListener *listener;
    int recursive;
    int tag;
    int pendingTransfers;

public:
    virtual void onRequestFinish(MegaApi* api, MegaRequest *request, MegaError *e);
    virtual void onTransferStart(MegaApi *api, MegaTransfer *transfer);
    virtual void onTransferUpdate(MegaApi *api, MegaTransfer *transfer);
    virtual void onTransferFinish(MegaApi* api, MegaTransfer *transfer, MegaError *e);
};


class MegaBackupController : public MegaBackup, public MegaRequestListener, public MegaTransferListener
{
public:
    MegaBackupController(MegaApiImpl *megaApi, int tag, int folderTransferTag, handle parenthandle, const char *filename, bool attendPastBackups, const char *speriod, int64_t period=-1, int maxBackups = 10);
    MegaBackupController(MegaBackupController *backup);
    ~MegaBackupController();

    void update();
    void start(bool skip = false);
    void removeexceeding();
    void abortCurrent();

    // MegaBackup interface
    MegaBackup *copy();
    const char *getLocalFolder() const;
    MegaHandle getMegaHandle() const;
    int getTag() const;
    int64_t getPeriod() const;
    const char *getPeriodString() const;
    int getMaxBackups() const;
    int getState() const;
    long long getNextStartTime(long long oldStartTimeAbsolute = -1) const;
    bool getAttendPastBackups() const;

    // MegaBackup setters
    void setLocalFolder(const std::string &value);
    void setMegaHandle(const MegaHandle &value);
    void setTag(int value);
    void setPeriod(const int64_t &value);
    void setPeriodstring(const std::string &value);
    void setMaxBackups(int value);
    void setState(int value);
    void setAttendPastBackups(bool value);

    //getters&setters
    int64_t getStartTime() const;
    void setStartTime(const int64_t &value);
    std::string getBackupName() const;
    void setBackupName(const std::string &value);
    int64_t getOffsetds() const;
    void setOffsetds(const int64_t &value);
    int64_t getLastbackuptime() const;
    void setLastbackuptime(const int64_t &value);
    int getFolderTransferTag() const;
    void setFolderTransferTag(int value);

    //convenience methods
    bool isBackup(std::string localname, std::string backupname) const;
    int64_t getTimeOfBackup(std::string localname) const;

protected:

    // common variables
    MegaApiImpl *megaApi;
    MegaClient *client;
    MegaBackupListener *backupListener;

    int state;
    int tag;
    int64_t lastwakeuptime;
    int64_t lastbackuptime; //ds absolute
    int pendingremovals;
    int folderTransferTag; //reused between backup instances
    std::string basepath;
    std::string backupName;
    handle parenthandle;
    int maxBackups;
    int64_t period;
    std::string periodstring;
    cron_expr ccronexpr;
    bool valid;
    int64_t offsetds; //times offset with epoch time?
    int64_t startTime; // when shall the next backup begin
    bool attendPastBackups;

    // backup instance related
    handle currentHandle;
    std::string currentName;
    std::list<std::string> pendingFolders;
    std::list<MegaTransfer *> failedTransfers;
    int recursive;
    int pendingTransfers;
    int pendingTags;
    // backup instance stats
    int64_t currentBKStartTime;
    int64_t updateTime;
    long long transferredBytes;
    long long totalBytes;
    long long speed;
    long long meanSpeed;
    long long numberFiles; //number of files successfully uploaded
    long long totalFiles;
    long long numberFolders;


    // internal methods
    void onFolderAvailable(MegaHandle handle);
    bool checkCompletion();
    bool isBusy() const;
    int64_t getLastBackupTime();
    long long getNextStartTimeDs(long long oldStartTimeds = -1) const;

    std::string epochdsToString(int64_t rawtimeds) const;
    int64_t stringTimeTods(string stime) const;

    void clearCurrentBackupData();

public:
    virtual void onRequestFinish(MegaApi* api, MegaRequest *request, MegaError *e);
    virtual void onTransferStart(MegaApi *api, MegaTransfer *transfer);
    virtual void onTransferUpdate(MegaApi *api, MegaTransfer *transfer);
    virtual void onTransferFinish(MegaApi* api, MegaTransfer *transfer, MegaError *e);

    long long getNumberFolders() const;
    void setNumberFolders(long long value);
    long long getNumberFiles() const;
    void setNumberFiles(long long value);
    long long getMeanSpeed() const;
    void setMeanSpeed(long long value);
    long long getSpeed() const;
    void setSpeed(long long value);
    long long getTotalBytes() const;
    void setTotalBytes(long long value);
    long long getTransferredBytes() const;
    void setTransferredBytes(long long value);
    int64_t getUpdateTime() const;
    void setUpdateTime(const int64_t &value);
    int64_t getCurrentBKStartTime() const;
    void setCurrentBKStartTime(const int64_t &value);
    long long getTotalFiles() const;
    void setTotalFiles(long long value);
    MegaBackupListener *getBackupListener() const;
    void setBackupListener(MegaBackupListener *value);
    cron_expr getCcronexpr() const;
    void setCcronexpr(const cron_expr &value);
    bool isValid() const;
    void setValid(bool value);
};

class MegaFolderDownloadController : public MegaTransferListener
{
public:
    MegaFolderDownloadController(MegaApiImpl *megaApi, MegaTransferPrivate *transfer);
    void start(MegaNode *node);

protected:
    void downloadFolderNode(MegaNode *node, string *path);
    void checkCompletion();

    MegaApiImpl *megaApi;
    MegaClient *client;
    MegaTransferPrivate *transfer;
    MegaTransferListener *listener;
    int recursive;
    int tag;
    int pendingTransfers;
    error e;

public:
    virtual void onTransferStart(MegaApi *, MegaTransfer *t);
    virtual void onTransferUpdate(MegaApi *, MegaTransfer *t);
    virtual void onTransferFinish(MegaApi*, MegaTransfer *t, MegaError *e);
};

class MegaNodePrivate : public MegaNode, public Cachable
{
    public:
        MegaNodePrivate(const char *name, int type, int64_t size, int64_t ctime, int64_t mtime,
                        MegaHandle nodeMegaHandle, std::string *nodekey, std::string *attrstring, std::string *fileattrstring,
                        const char *fingerprint, MegaHandle parentHandle = INVALID_HANDLE,
                        const char *privateauth = NULL, const char *publicauth = NULL, bool isPublic = true,
                        bool isForeign = false, const char *chatauth = NULL);

        MegaNodePrivate(MegaNode *node);
        virtual ~MegaNodePrivate();
        virtual int getType();
        virtual const char* getName();
        virtual const char* getFingerprint();
        virtual bool hasCustomAttrs();
        MegaStringList *getCustomAttrNames();
        virtual const char *getCustomAttr(const char* attrName);
        virtual int getDuration();
        virtual int getWidth();
        virtual int getHeight();
        virtual int getShortformat();
        virtual int getVideocodecid();
        virtual double getLatitude();
        virtual double getLongitude();
        virtual char *getBase64Handle();
        virtual int64_t getSize();
        virtual int64_t getCreationTime();
        virtual int64_t getModificationTime();
        virtual MegaHandle getHandle();
        virtual MegaHandle getRestoreHandle();
        virtual MegaHandle getParentHandle();
        virtual std::string* getNodeKey();
        virtual char *getBase64Key();
        virtual std::string* getAttrString();
        virtual char* getFileAttrString();
        virtual int getTag();
        virtual int64_t getExpirationTime();
        virtual MegaHandle getPublicHandle();
        virtual MegaNode* getPublicNode();
        virtual char *getPublicLink(bool includeKey = true);
        virtual bool isFile();
        virtual bool isFolder();
        virtual bool isRemoved();
        virtual bool hasChanged(int changeType);
        virtual int getChanges();
        virtual bool hasThumbnail();
        virtual bool hasPreview();
        virtual bool isPublic();
        virtual bool isExported();
        virtual bool isExpired();
        virtual bool isTakenDown();
        virtual bool isForeign();
        virtual std::string* getPrivateAuth();
        virtual MegaNodeList *getChildren();
        virtual void setPrivateAuth(const char *privateAuth);
        void setPublicAuth(const char *publicAuth);
        void setChatAuth(const char *chatAuth);
        void setForeign(bool foreign);
        void setChildren(MegaNodeList *children);
        void setName(const char *newName);
        virtual std::string* getPublicAuth();
        virtual const char *getChatAuth();
        virtual bool isShared();
        virtual bool isOutShare();
        virtual bool isInShare();
        std::string* getSharekey();


#ifdef ENABLE_SYNC
        virtual bool isSyncDeleted();
        virtual std::string getLocalPath();
#endif

        static MegaNode *fromNode(Node *node);
        virtual MegaNode *copy();

        virtual char *serialize();
        virtual bool serialize(string*);
        static MegaNodePrivate* unserialize(string*);

    protected:
        MegaNodePrivate(Node *node);
        int type;
        const char *name;
        const char *fingerprint;
        attr_map *customAttrs;
        int64_t size;
        int64_t ctime;
        int64_t mtime;
        MegaHandle nodehandle;
        MegaHandle parenthandle;
        MegaHandle restorehandle;
        std::string nodekey;
        std::string attrstring;
        std::string fileattrstring;
        std::string privateAuth;
        std::string publicAuth;
        const char *chatAuth;
        int tag;
        int changed;
        struct {
            bool thumbnailAvailable : 1;
            bool previewAvailable : 1;
            bool isPublicNode : 1;
            bool outShares : 1;
            bool inShare : 1;
            bool foreign : 1;
        };
        PublicLink *plink;
        std::string *sharekey;   // for plinks of folders
        int duration;
        int width;
        int height;
        int shortformat;
        int videocodecid;
        double latitude;
        double longitude;
        MegaNodeList *children;

#ifdef ENABLE_SYNC
        bool syncdeleted;
        std::string localPath;
#endif
};


class MegaUserPrivate : public MegaUser
{
	public:
		MegaUserPrivate(User *user);
		MegaUserPrivate(MegaUser *user);
		static MegaUser *fromUser(User *user);
		virtual MegaUser *copy();

		~MegaUserPrivate();
		virtual const char* getEmail();
        virtual MegaHandle getHandle();
        virtual int getVisibility();
        virtual int64_t getTimestamp();
        virtual bool hasChanged(int changeType);
        virtual int getChanges();
        virtual int isOwnChange();

	protected:
		const char *email;
        MegaHandle handle;
        int visibility;
        int64_t ctime;
        int changed;
        int tag;
};

class MegaUserAlertPrivate : public MegaUserAlert
{
public:
    MegaUserAlertPrivate(UserAlert::Base* user, MegaClient* mc);
    //MegaUserAlertPrivate(const MegaUserAlertPrivate&); // default copy works for this type
    virtual MegaUserAlert* copy() const;

    virtual unsigned getId() const;
    virtual bool getSeen() const;
    virtual bool getRelevant() const;
    virtual int getType() const;
    virtual const char *getTypeString() const;
    virtual MegaHandle getUserHandle() const;
    virtual MegaHandle getNodeHandle() const;
    virtual const char* getEmail() const;
    virtual const char* getPath() const;
    virtual const char* getName() const;
    virtual const char* getHeading() const;
    virtual const char* getTitle() const;
    virtual int64_t getNumber(unsigned index) const;
    virtual int64_t getTimestamp(unsigned index) const;
    virtual const char* getString(unsigned index) const;
    virtual bool isOwnChange() const;

protected:
    unsigned id;
    bool seen;
    bool relevant;
    int type;
    int tag;
    string heading;
    string title;
    handle userHandle;
    string email;
    handle nodeHandle;
    string nodePath;
    string nodeName;
    vector<int64_t> numbers;
    vector<int64_t> timestamps;
    vector<string> extraStrings;
};

class MegaHandleListPrivate : public MegaHandleList
{
public:
    MegaHandleListPrivate();
    MegaHandleListPrivate(const MegaHandleListPrivate *hList);
    virtual ~MegaHandleListPrivate();

    virtual MegaHandleList *copy() const;
    virtual MegaHandle get(unsigned int i) const;
    virtual unsigned int size() const;
    virtual void addMegaHandle(MegaHandle megaHandle);

private:
    std::vector<MegaHandle> mList;
};

class MegaSharePrivate : public MegaShare
{
	public:
        static MegaShare *fromShare(MegaHandle nodeMegaHandle, Share *share, bool pending = false);
		virtual MegaShare *copy();
		virtual ~MegaSharePrivate();
		virtual const char *getUser();
		virtual MegaHandle getNodeHandle();
		virtual int getAccess();
		virtual int64_t getTimestamp();
        virtual bool isPending();

	protected:
        MegaSharePrivate(MegaHandle nodehandle, Share *share, bool pending = false);
		MegaSharePrivate(MegaShare *share);

		MegaHandle nodehandle;
		const char *user;
		int access;
		int64_t ts;
        bool pending;
};

class MegaTransferPrivate : public MegaTransfer, public Cachable
{
	public:
		MegaTransferPrivate(int type, MegaTransferListener *listener = NULL);
        MegaTransferPrivate(const MegaTransferPrivate *transfer);
        virtual ~MegaTransferPrivate();
        
        virtual MegaTransfer *copy();
	    Transfer *getTransfer() const;
        void setTransfer(Transfer *transfer); 
        void setStartTime(int64_t startTime);
		void setTransferredBytes(long long transferredBytes);
		void setTotalBytes(long long totalBytes);
		void setPath(const char* path);
		void setParentPath(const char* path);
        void setNodeHandle(MegaHandle nodeHandle);
        void setParentHandle(MegaHandle parentHandle);
		void setNumConnections(int connections);
		void setStartPos(long long startPos);
		void setEndPos(long long endPos);
		void setNumRetry(int retry);
		void setMaxRetries(int retry);
        void setTime(int64_t time);
		void setFileName(const char* fileName);
		void setSlot(int id);
		void setTag(int tag);
		void setSpeed(long long speed);
        void setMeanSpeed(long long meanSpeed);
		void setDeltaSize(long long deltaSize);
        void setUpdateTime(int64_t updateTime);
        void setPublicNode(MegaNode *publicNode, bool copyChildren = false);
        void setSyncTransfer(bool syncTransfer);
        void setSourceFileTemporary(bool temporary);
        void setStartFirst(bool startFirst);
        void setStreamingTransfer(bool streamingTransfer);
        void setLastBytes(char *lastBytes);
        void setLastError(MegaError e);
        void setFolderTransferTag(int tag);
        void setNotificationNumber(long long notificationNumber);
        void setListener(MegaTransferListener *listener);

		virtual int getType() const;
		virtual const char * getTransferString() const;
		virtual const char* toString() const;
		virtual const char* __str__() const;
		virtual const char* __toString() const;
        virtual int64_t getStartTime() const;
		virtual long long getTransferredBytes() const;
		virtual long long getTotalBytes() const;
		virtual const char* getPath() const;
		virtual const char* getParentPath() const;
        virtual MegaHandle getNodeHandle() const;
        virtual MegaHandle getParentHandle() const;
		virtual long long getStartPos() const;
		virtual long long getEndPos() const;
		virtual const char* getFileName() const;
		virtual MegaTransferListener* getListener() const;
		virtual int getNumRetry() const;
		virtual int getMaxRetries() const;
        virtual int64_t getTime() const;
		virtual int getTag() const;
		virtual long long getSpeed() const;
        virtual long long getMeanSpeed() const;
		virtual long long getDeltaSize() const;
        virtual int64_t getUpdateTime() const;
        virtual MegaNode *getPublicNode() const;
        virtual MegaNode *getPublicMegaNode() const;
        virtual bool isSyncTransfer() const;
        virtual bool isStreamingTransfer() const;
        virtual bool isFinished() const;
        virtual bool isSourceFileTemporary() const;
        virtual bool shouldStartFirst() const;
        virtual char *getLastBytes() const;
        virtual MegaError getLastError() const;
        virtual bool isFolderTransfer() const;
        virtual int getFolderTransferTag() const;
        virtual void setAppData(const char *data);
        virtual const char* getAppData() const;
        virtual void setState(int state);
        virtual int getState() const;
        virtual void setPriority(unsigned long long p);
        virtual unsigned long long getPriority() const;
        virtual long long getNotificationNumber() const;

        virtual bool serialize(string*);
        static MegaTransferPrivate* unserialize(string*);

    protected:
        int type;
        int tag;
        int state;
        uint64_t priority;

        struct
        {
            bool syncTransfer : 1;
            bool streamingTransfer : 1;
            bool temporarySourceFile : 1;
            bool startFirst : 1;
        };

        int64_t startTime;
        int64_t updateTime;
        int64_t time;
        long long transferredBytes;
        long long totalBytes;
        long long speed;
        long long meanSpeed;
        long long deltaSize;
        long long notificationNumber;
        MegaHandle nodeHandle;
        MegaHandle parentHandle;
        const char* path;
        const char* parentPath;
        const char* fileName;
        char *lastBytes;
        MegaNode *publicNode;
        long long startPos;
        long long endPos;
        int retry;
        int maxRetries;

        MegaTransferListener *listener;
        Transfer *transfer;
        MegaError lastError;
        int folderTransferTag;
        const char* appData;
};

class MegaTransferDataPrivate : public MegaTransferData
{
public:
    MegaTransferDataPrivate(TransferList *transferList, long long notificationNumber);
    MegaTransferDataPrivate(const MegaTransferDataPrivate *transferData);

    virtual ~MegaTransferDataPrivate();
    virtual MegaTransferData *copy() const;
    virtual int getNumDownloads() const;
    virtual int getNumUploads() const;
    virtual int getDownloadTag(int i) const;
    virtual int getUploadTag(int i) const;
    virtual unsigned long long getDownloadPriority(int i) const;
    virtual unsigned long long getUploadPriority(int i) const;
    virtual long long getNotificationNumber() const;

protected:
    int numDownloads;
    int numUploads;
    long long notificationNumber;
    vector<int> downloadTags;
    vector<int> uploadTags;
    vector<uint64_t> downloadPriorities;
    vector<uint64_t> uploadPriorities;
};

class MegaFolderInfoPrivate : public MegaFolderInfo
{
public:
    MegaFolderInfoPrivate(int numFiles, int numFolders, int numVersions, long long currentSize, long long versionsSize);
    MegaFolderInfoPrivate(const MegaFolderInfoPrivate *folderData);

    virtual ~MegaFolderInfoPrivate();

    virtual MegaFolderInfo *copy() const;

    virtual int getNumVersions() const;
    virtual int getNumFiles() const;
    virtual int getNumFolders() const;
    virtual long long getCurrentSize() const;
    virtual long long getVersionsSize() const;

protected:
    int numFiles;
    int numFolders;
    int numVersions;
    long long currentSize;
    long long versionsSize;
};

class MegaTimeZoneDetailsPrivate : public MegaTimeZoneDetails
{
public:
    MegaTimeZoneDetailsPrivate(vector<string>* timeZones, vector<int> *timeZoneOffsets, int defaultTimeZone);
    MegaTimeZoneDetailsPrivate(const MegaTimeZoneDetailsPrivate *timeZoneDetails);

    virtual ~MegaTimeZoneDetailsPrivate();
    virtual MegaTimeZoneDetails *copy() const;

    virtual int getNumTimeZones() const;
    virtual const char *getTimeZone(int index) const;
    virtual int getTimeOffset(int index) const;
    virtual int getDefault() const;

protected:
    int defaultTimeZone;
    vector<string> timeZones;
    vector<int> timeZoneOffsets;
};


class MegaContactRequestPrivate : public MegaContactRequest
{
public:
    MegaContactRequestPrivate(PendingContactRequest *request);
    MegaContactRequestPrivate(const MegaContactRequest *request);
    virtual ~MegaContactRequestPrivate();

    static MegaContactRequest *fromContactRequest(PendingContactRequest *request);
    virtual MegaContactRequest *copy() const;

    virtual MegaHandle getHandle() const;
    virtual char* getSourceEmail() const;
    virtual char* getSourceMessage() const;
    virtual char* getTargetEmail() const;
    virtual int64_t getCreationTime() const;
    virtual int64_t getModificationTime() const;
    virtual int getStatus() const;
    virtual bool isOutgoing() const;
    virtual bool isAutoAccepted() const;

protected:
    MegaHandle handle;
    char* sourceEmail;
    char* sourceMessage;
    char* targetEmail;
    int64_t creationTime;
    int64_t modificationTime;
    int status;
    bool outgoing;
    bool autoaccepted;
};

#ifdef ENABLE_SYNC

class MegaSyncEventPrivate: public MegaSyncEvent
{
public:
    MegaSyncEventPrivate(int type);
    virtual ~MegaSyncEventPrivate();

    virtual MegaSyncEvent *copy();

    virtual int getType() const;
    virtual const char *getPath() const;
    virtual MegaHandle getNodeHandle() const;
    virtual const char *getNewPath() const;
    virtual const char* getPrevName() const;
    virtual MegaHandle getPrevParent() const;

    void setPath(const char* path);
    void setNodeHandle(MegaHandle nodeHandle);
    void setNewPath(const char* newPath);
    void setPrevName(const char* prevName);
    void setPrevParent(MegaHandle prevParent);

protected:
    int type;
    const char* path;
    const char* newPath;
    const char* prevName;
    MegaHandle nodeHandle;
    MegaHandle prevParent;
};

class MegaRegExpPrivate
{
public:
    MegaRegExpPrivate();
    ~MegaRegExpPrivate();

    MegaRegExpPrivate *copy();

    bool addRegExp(const char *regExp);
    int getNumRegExp();
    const char *getRegExp(int index);
    bool match(const char *s);
    const char *getFullPattern();

private:
    enum{
        REGEXP_NO_ERROR = 0,
        REGEXP_COMPILATION_ERROR,
        REGEXP_OPTIMIZATION_ERROR,
        REGEXP_EMPTY
    };
    int compile();
    bool updatePattern();
    bool checkRegExp(const char *regExp);
    bool isPatternUpdated();

private:
    std::vector<std::string> regExps;
    std::string pattern;
    bool patternUpdated;

#ifdef USE_PCRE
    int options;
    pcre* reCompiled;
    pcre_extra* reOptimization;
#endif
};

class MegaSyncPrivate : public MegaSync
{  
public:
    MegaSyncPrivate(const char *path, handle nodehandle, int tag);
    MegaSyncPrivate(MegaSyncPrivate *sync);

    virtual ~MegaSyncPrivate();

    virtual MegaSync *copy();

    virtual MegaHandle getMegaHandle() const;
    void setMegaHandle(MegaHandle handle);
    virtual const char* getLocalFolder() const;
    void setLocalFolder(const char*path);
    virtual long long getLocalFingerprint() const;
    void setLocalFingerprint(long long fingerprint);
    virtual int getTag() const;
    void setTag(int tag);
    void setListener(MegaSyncListener *listener);
    MegaSyncListener *getListener();
    virtual int getState() const;
    void setState(int state);
    virtual MegaRegExp* getRegExp() const;
    void setRegExp(MegaRegExp *regExp);

protected:
    MegaHandle megaHandle;
    char *localFolder;
    MegaRegExp *regExp;
    int tag;
    long long fingerprint;
    MegaSyncListener *listener;
    int state; 
};

#endif


class MegaPricingPrivate;
class MegaRequestPrivate : public MegaRequest
{
	public:
        MegaRequestPrivate(int type, MegaRequestListener *listener = NULL);
        MegaRequestPrivate(MegaRequestPrivate *request);

        virtual ~MegaRequestPrivate();
        MegaRequest *copy();
        void setNodeHandle(MegaHandle nodeHandle);
        void setLink(const char* link);
        void setParentHandle(MegaHandle parentHandle);
        void setSessionKey(const char* sessionKey);
        void setName(const char* name);
        void setEmail(const char* email);
        void setPassword(const char* email);
        void setNewPassword(const char* email);
        void setPrivateKey(const char* privateKey);
        void setAccess(int access);
        void setNumRetry(int ds);
        void setNextRetryDelay(int delay);
        void setPublicNode(MegaNode* publicNode, bool copyChildren = false);
        void setNumDetails(int numDetails);
        void setFile(const char* file);
        void setParamType(int type);
        void setText(const char* text);
        void setNumber(long long number);
        void setFlag(bool flag);
        void setTransferTag(int transfer);
        void setListener(MegaRequestListener *listener);
        void setTotalBytes(long long totalBytes);
        void setTransferredBytes(long long transferredBytes);
        void setTag(int tag);
        void addProduct(handle product, int proLevel, unsigned int gbStorage, unsigned int gbTransfer,
                        int months, int amount, const char *currency, const char *description, const char *iosid, const char *androidid);
        void setProxy(Proxy *proxy);
        Proxy *getProxy();
        void setTimeZoneDetails(MegaTimeZoneDetails *timeZoneDetails);

        virtual int getType() const;
        virtual const char *getRequestString() const;
        virtual const char* toString() const;
        virtual const char* __str__() const;
        virtual const char* __toString() const;
        virtual MegaHandle getNodeHandle() const;
        virtual const char* getLink() const;
        virtual MegaHandle getParentHandle() const;
        virtual const char* getSessionKey() const;
        virtual const char* getName() const;
        virtual const char* getEmail() const;
        virtual const char* getPassword() const;
        virtual const char* getNewPassword() const;
        virtual const char* getPrivateKey() const;
        virtual int getAccess() const;
        virtual const char* getFile() const;
        virtual int getNumRetry() const;
        virtual MegaNode *getPublicNode() const;
        virtual MegaNode *getPublicMegaNode() const;
        virtual int getParamType() const;
        virtual const char *getText() const;
        virtual long long getNumber() const;
        virtual bool getFlag() const;
        virtual long long getTransferredBytes() const;
        virtual long long getTotalBytes() const;
        virtual MegaRequestListener *getListener() const;
        virtual MegaAccountDetails *getMegaAccountDetails() const;
        virtual int getTransferTag() const;
        virtual int getNumDetails() const;
        virtual int getTag() const;
        virtual MegaPricing *getPricing() const;
        AccountDetails * getAccountDetails() const;
        virtual MegaAchievementsDetails *getMegaAchievementsDetails() const;
        AchievementsDetails *getAchievementsDetails() const;
        MegaTimeZoneDetails *getMegaTimeZoneDetails () const;

#ifdef ENABLE_CHAT
        virtual MegaTextChatPeerList *getMegaTextChatPeerList() const;
        void setMegaTextChatPeerList(MegaTextChatPeerList *chatPeers);
        virtual MegaTextChatList *getMegaTextChatList() const;
        void setMegaTextChatList(MegaTextChatList *chatList);
#endif
        virtual MegaStringMap *getMegaStringMap() const;
        void setMegaStringMap(const MegaStringMap *);
        virtual MegaFolderInfo *getMegaFolderInfo() const;
        void setMegaFolderInfo(const MegaFolderInfo *);

#ifdef ENABLE_SYNC
        void setSyncListener(MegaSyncListener *syncListener);
        MegaSyncListener *getSyncListener() const;
        void setRegExp(MegaRegExp *regExp);
        virtual MegaRegExp *getRegExp() const;
#endif

        MegaBackupListener *getBackupListener() const;
        void setBackupListener(MegaBackupListener *value);

protected:
        AccountDetails *accountDetails;
        MegaPricingPrivate *megaPricing;
        AchievementsDetails *achievementsDetails;
        MegaTimeZoneDetails *timeZoneDetails;
        int type;
        MegaHandle nodeHandle;
        const char* link;
        const char* name;
        MegaHandle parentHandle;
        const char* sessionKey;
        const char* email;
        const char* password;
        const char* newPassword;
        const char* privateKey;
        const char* text;
        long long number;
        int access;
        const char* file;
        int attrType;
        bool flag;
        long long totalBytes;
        long long transferredBytes;
        MegaRequestListener *listener;
#ifdef ENABLE_SYNC
        MegaSyncListener *syncListener;
        MegaRegExp *regExp;
#endif
        MegaBackupListener *backupListener;

        int transfer;
        int numDetails;
        MegaNode* publicNode;
        int numRetry;
        int tag;
        Proxy *proxy;

#ifdef ENABLE_CHAT
        MegaTextChatPeerList *chatPeerList;
        MegaTextChatList *chatList;
#endif
        MegaStringMap *stringMap;
        MegaFolderInfo *folderInfo;
};

class MegaEventPrivate : public MegaEvent
{
public:
    MegaEventPrivate(int type);
    MegaEventPrivate(MegaEventPrivate *event);
    virtual ~MegaEventPrivate();
    MegaEvent *copy();

    virtual int getType() const;
    virtual const char *getText() const;
    virtual const int getNumber() const;

    void setText(const char* text);
    void setNumber(int number);

protected:
    int type;
    const char* text;
    int number;
};

class MegaAccountBalancePrivate : public MegaAccountBalance
{
public:
    static MegaAccountBalance *fromAccountBalance(const AccountBalance *balance);
    virtual ~MegaAccountBalancePrivate() ;
    virtual MegaAccountBalance* copy();

    virtual double getAmount() const;
    virtual char* getCurrency() const;

protected:
    MegaAccountBalancePrivate(const AccountBalance *balance);
    AccountBalance balance;
};

class MegaAccountSessionPrivate : public MegaAccountSession
{
public:
    static MegaAccountSession *fromAccountSession(const AccountSession *session);
    virtual ~MegaAccountSessionPrivate() ;
    virtual MegaAccountSession* copy();

    virtual int64_t getCreationTimestamp() const;
    virtual int64_t getMostRecentUsage() const;
    virtual char *getUserAgent() const;
    virtual char *getIP() const;
    virtual char *getCountry() const;
    virtual bool isCurrent() const;
    virtual bool isAlive() const;
    virtual MegaHandle getHandle() const;

private:
    MegaAccountSessionPrivate(const AccountSession *session);
    AccountSession session;
};

class MegaAccountPurchasePrivate : public MegaAccountPurchase
{
public:   
    static MegaAccountPurchase *fromAccountPurchase(const AccountPurchase *purchase);
    virtual ~MegaAccountPurchasePrivate() ;
    virtual MegaAccountPurchase* copy();

    virtual int64_t getTimestamp() const;
    virtual char *getHandle() const;
    virtual char *getCurrency() const;
    virtual double getAmount() const;
    virtual int getMethod() const;

private:
    MegaAccountPurchasePrivate(const AccountPurchase *purchase);
    AccountPurchase purchase;
};

class MegaAccountTransactionPrivate : public MegaAccountTransaction
{
public:
    static MegaAccountTransaction *fromAccountTransaction(const AccountTransaction *transaction);
    virtual ~MegaAccountTransactionPrivate() ;
    virtual MegaAccountTransaction* copy();

    virtual int64_t getTimestamp() const;
    virtual char *getHandle() const;
    virtual char *getCurrency() const;
    virtual double getAmount() const;

private:
    MegaAccountTransactionPrivate(const AccountTransaction *transaction);
    AccountTransaction transaction;
};

class MegaAccountDetailsPrivate : public MegaAccountDetails
{
    public:
        static MegaAccountDetails *fromAccountDetails(AccountDetails *details);
        virtual ~MegaAccountDetailsPrivate();

        virtual int getProLevel();
        virtual int64_t getProExpiration();
        virtual int getSubscriptionStatus();
        virtual int64_t getSubscriptionRenewTime();
        virtual char* getSubscriptionMethod();
        virtual char* getSubscriptionCycle();

        virtual long long getStorageMax();
        virtual long long getStorageUsed();
        virtual long long getVersionStorageUsed();
        virtual long long getTransferMax();
        virtual long long getTransferOwnUsed();

        virtual int getNumUsageItems();
        virtual long long getStorageUsed(MegaHandle handle);
        virtual long long getNumFiles(MegaHandle handle);
        virtual long long getNumFolders(MegaHandle handle);
        virtual long long getVersionStorageUsed(MegaHandle handle);
        virtual long long getNumVersionFiles(MegaHandle handle);

        virtual MegaAccountDetails* copy();

        virtual int getNumBalances() const;
        virtual MegaAccountBalance* getBalance(int i) const;

        virtual int getNumSessions() const;
        virtual MegaAccountSession* getSession(int i) const;

        virtual int getNumPurchases() const;
        virtual MegaAccountPurchase* getPurchase(int i) const;

        virtual int getNumTransactions() const;
        virtual MegaAccountTransaction* getTransaction(int i) const;

        virtual int getTemporalBandwidthInterval();
        virtual long long getTemporalBandwidth();
        virtual bool isTemporalBandwidthValid();

    private:
        MegaAccountDetailsPrivate(AccountDetails *details);
        AccountDetails details;
};

class MegaPricingPrivate : public MegaPricing
{
public:
    virtual ~MegaPricingPrivate();
    virtual int getNumProducts();
    virtual MegaHandle getHandle(int productIndex);
    virtual int getProLevel(int productIndex);
    virtual unsigned int getGBStorage(int productIndex);
    virtual unsigned int getGBTransfer(int productIndex);
    virtual int getMonths(int productIndex);
    virtual int getAmount(int productIndex);
    virtual const char* getCurrency(int productIndex);
    virtual const char* getDescription(int productIndex);
    virtual const char* getIosID(int productIndex);
    virtual const char* getAndroidID(int productIndex);
    virtual MegaPricing *copy();

    void addProduct(handle product, int proLevel, unsigned int gbStorage, unsigned int gbTransfer,
                    int months, int amount, const char *currency, const char *description, const char *iosid, const char *androidid);
private:
    vector<handle> handles;
    vector<int> proLevel;
    vector<unsigned int> gbStorage;
    vector<unsigned int> gbTransfer;
    vector<int> months;
    vector<int> amount;
    vector<const char *> currency;
    vector<const char *> description;
    vector<const char *> iosId;
    vector<const char *> androidId;
};

class MegaAchievementsDetailsPrivate : public MegaAchievementsDetails
{
public:
    static MegaAchievementsDetails *fromAchievementsDetails(AchievementsDetails *details);
    virtual ~MegaAchievementsDetailsPrivate();

    virtual MegaAchievementsDetails* copy();

    virtual long long getBaseStorage();
    virtual long long getClassStorage(int class_id);
    virtual long long getClassTransfer(int class_id);
    virtual int getClassExpire(int class_id);
    virtual unsigned int getAwardsCount();
    virtual int getAwardClass(unsigned int index);
    virtual int getAwardId(unsigned int index);
    virtual int64_t getAwardTimestamp(unsigned int index);
    virtual int64_t getAwardExpirationTs(unsigned int index);
    virtual MegaStringList* getAwardEmails(unsigned int index);
    virtual int getRewardsCount();
    virtual int getRewardAwardId(unsigned int index);
    virtual long long getRewardStorage(unsigned int index);
    virtual long long getRewardTransfer(unsigned int index);
    virtual long long getRewardStorageByAwardId(int award_id);
    virtual long long getRewardTransferByAwardId(int award_id);
    virtual int getRewardExpire(unsigned int index);

    virtual long long currentStorage();
    virtual long long currentTransfer();
    virtual long long currentStorageReferrals();
    virtual long long currentTransferReferrals();

private:
    MegaAchievementsDetailsPrivate(AchievementsDetails *details);
    AchievementsDetails details;
};

#ifdef ENABLE_CHAT
class MegaTextChatPeerListPrivate : public MegaTextChatPeerList
{
public:
    MegaTextChatPeerListPrivate();
    MegaTextChatPeerListPrivate(userpriv_vector *);

    virtual ~MegaTextChatPeerListPrivate();
    virtual MegaTextChatPeerList *copy() const;
    virtual void addPeer(MegaHandle h, int priv);
    virtual MegaHandle getPeerHandle(int i) const;
    virtual int getPeerPrivilege(int i) const;
    virtual int size() const;

    // returns the list of user-privilege (this object keeps the ownership)
    const userpriv_vector * getList() const;

    void setPeerPrivilege(handle uh, privilege_t priv);

private:
    userpriv_vector list;
};

class MegaTextChatPrivate : public MegaTextChat
{
public:
    MegaTextChatPrivate(const MegaTextChat *);
    MegaTextChatPrivate(const TextChat *);

    virtual ~MegaTextChatPrivate();
    virtual MegaTextChat *copy() const;

    virtual MegaHandle getHandle() const;
    virtual int getOwnPrivilege() const;
    virtual int getShard() const;
    virtual const MegaTextChatPeerList *getPeerList() const;
    virtual void setPeerList(const MegaTextChatPeerList *peers);
    virtual bool isGroup() const;
    virtual MegaHandle getOriginatingUser() const;
    virtual const char *getTitle() const;
    virtual const char *getUnifiedKey() const;
    virtual int64_t getCreationTime() const;
    virtual bool isArchived() const;
    virtual bool isPublicChat() const;

    virtual bool hasChanged(int changeType) const;
    virtual int getChanges() const;
    virtual int isOwnChange() const;

private:
    handle id;
    int priv;
    string url;
    int shard;
    MegaTextChatPeerList *peers;
    bool group;
    handle ou;
    string title;
    string unifiedKey;
    int changed;
    int tag;
    bool archived;
    bool publicchat;
    int64_t ts;
};

class MegaTextChatListPrivate : public MegaTextChatList
{
public:
    MegaTextChatListPrivate();
    MegaTextChatListPrivate(textchat_map *list);

    virtual ~MegaTextChatListPrivate();
    virtual MegaTextChatList *copy() const;
    virtual const MegaTextChat *get(unsigned int i) const;
    virtual int size() const;

    void addChat(MegaTextChatPrivate*);

private:
    MegaTextChatListPrivate(const MegaTextChatListPrivate*);
    vector<MegaTextChat*> list;
};

#endif

class MegaStringMapPrivate : public MegaStringMap
{
public:
    MegaStringMapPrivate();
    MegaStringMapPrivate(const string_map *map, bool toBase64 = false);
    virtual ~MegaStringMapPrivate();
    virtual MegaStringMap *copy() const;
    virtual const char *get(const char* key) const;
    virtual MegaStringList *getKeys() const;
    virtual void set(const char *key, const char *value);
    virtual int size() const;
    const string_map *getMap() const;

protected:
    MegaStringMapPrivate(const MegaStringMapPrivate *megaStringMap);
    string_map strMap;
};


class MegaStringListPrivate : public MegaStringList
{
public:
    MegaStringListPrivate();
    MegaStringListPrivate(char **newlist, int size);
    virtual ~MegaStringListPrivate();
    virtual MegaStringList *copy();
    virtual const char* get(int i);
    virtual int size();


protected:
    MegaStringListPrivate(MegaStringListPrivate *stringList);
    const char** list;
    int s;
};

class MegaNodeListPrivate : public MegaNodeList
{
	public:
        MegaNodeListPrivate();
        MegaNodeListPrivate(Node** newlist, int size);
        MegaNodeListPrivate(MegaNodeListPrivate *nodeList, bool copyChildren = false);
        virtual ~MegaNodeListPrivate();
		virtual MegaNodeList *copy();
		virtual MegaNode* get(int i);
		virtual int size();

        virtual void addNode(MegaNode* node);
	
	protected:
		MegaNode** list;
		int s;
};

class MegaChildrenListsPrivate : public MegaChildrenLists
{
    public:
        MegaChildrenListsPrivate();
        MegaChildrenListsPrivate(MegaChildrenLists*);
        MegaChildrenListsPrivate(MegaNodeListPrivate *folderList, MegaNodeListPrivate *fileList);
        virtual ~MegaChildrenListsPrivate();
        virtual MegaChildrenLists *copy();
        virtual MegaNodeList* getFolderList();
        virtual MegaNodeList* getFileList();

    protected:
        MegaNodeList *folders;
        MegaNodeList *files;
};

class MegaUserListPrivate : public MegaUserList
{
	public:
        MegaUserListPrivate();
        MegaUserListPrivate(User** newlist, int size);
        virtual ~MegaUserListPrivate();
		virtual MegaUserList *copy();
		virtual MegaUser* get(int i);
		virtual int size();
	
	protected:
        MegaUserListPrivate(MegaUserListPrivate *userList);
		MegaUser** list;
		int s;
};

class MegaShareListPrivate : public MegaShareList
{
	public:
        MegaShareListPrivate();
        MegaShareListPrivate(Share** newlist, MegaHandle *MegaHandlelist, int size, bool pending = false);
        virtual ~MegaShareListPrivate();
		virtual MegaShare* get(int i);
		virtual int size();
		
	protected:
		MegaShare** list;
		int s;
};

class MegaTransferListPrivate : public MegaTransferList
{
	public:
        MegaTransferListPrivate();
        MegaTransferListPrivate(MegaTransfer** newlist, int size);
        virtual ~MegaTransferListPrivate();
		virtual MegaTransfer* get(int i);
		virtual int size();
	
	protected:
		MegaTransfer** list;
		int s;
};

class MegaContactRequestListPrivate : public MegaContactRequestList
{
    public:
        MegaContactRequestListPrivate();
        MegaContactRequestListPrivate(PendingContactRequest ** newlist, int size);
        virtual ~MegaContactRequestListPrivate();
        virtual MegaContactRequestList *copy();
        virtual MegaContactRequest* get(int i);
        virtual int size();

    protected:
        MegaContactRequestListPrivate(MegaContactRequestListPrivate *requestList);
        MegaContactRequest** list;
        int s;
};

class MegaUserAlertListPrivate : public MegaUserAlertList
{
public:
    MegaUserAlertListPrivate();
    MegaUserAlertListPrivate(UserAlert::Base** newlist, int size, MegaClient* mc);
    MegaUserAlertListPrivate(const MegaUserAlertListPrivate &userList);
    virtual ~MegaUserAlertListPrivate();
    virtual MegaUserAlertList *copy() const;
    virtual MegaUserAlert* get(int i) const;
    virtual int size() const;

protected:
    MegaUserAlertListPrivate(MegaUserAlertListPrivate *userList);
    MegaUserAlert** list;
    int s;
};


struct MegaFile : public File
{
    MegaFile();

    void setTransfer(MegaTransferPrivate *transfer);
    MegaTransferPrivate *getTransfer();
    virtual bool serialize(string*);

    static MegaFile* unserialize(string*);

protected:
    MegaTransferPrivate *megaTransfer;
};

struct MegaFileGet : public MegaFile
{
    void prepare();
    void updatelocalname();
    void progress();
    void completed(Transfer*, LocalNode*);
    void terminated();
	MegaFileGet(MegaClient *client, Node* n, string dstPath);
    MegaFileGet(MegaClient *client, MegaNode* n, string dstPath);
    ~MegaFileGet() {}

    virtual bool serialize(string*);
    static MegaFileGet* unserialize(string*);

private:
    MegaFileGet() {}
};

struct MegaFilePut : public MegaFile
{
    void completed(Transfer* t, LocalNode*);
    void terminated();
    MegaFilePut(MegaClient *client, string* clocalname, string *filename, handle ch, const char* ctargetuser, int64_t mtime = -1, bool isSourceTemporary = false);
    ~MegaFilePut() {}

    virtual bool serialize(string*);
    static MegaFilePut* unserialize(string*);

protected:
    int64_t customMtime;

private:
    MegaFilePut() {}
};

class TreeProcessor
{
    public:
        virtual bool processNode(Node* node);
        virtual ~TreeProcessor();
};

class SearchTreeProcessor : public TreeProcessor
{
    public:
        SearchTreeProcessor(const char *search);
        virtual bool processNode(Node* node);
        virtual ~SearchTreeProcessor() {}
        vector<Node *> &getResults();

    protected:
        const char *search;
        vector<Node *> results;
};

class OutShareProcessor : public TreeProcessor
{
    public:
        OutShareProcessor();
        virtual bool processNode(Node* node);
        virtual ~OutShareProcessor() {}
        vector<Share *> &getShares();
        vector<handle> &getHandles();

    protected:
        vector<Share *> shares;
        vector<handle> handles;
};

class PendingOutShareProcessor : public TreeProcessor
{
    public:
        PendingOutShareProcessor();
        virtual bool processNode(Node* node);
        virtual ~PendingOutShareProcessor() {}
        vector<Share *> &getShares();
        vector<handle> &getHandles();

    protected:
        vector<Share *> shares;
        vector<handle> handles;
};

class PublicLinkProcessor : public TreeProcessor
{
    public:
        PublicLinkProcessor();
        virtual bool processNode(Node* node);
        virtual ~PublicLinkProcessor();
        vector<Node *> &getNodes();

    protected:
        vector<Node *> nodes;
};

class SizeProcessor : public TreeProcessor
{
    protected:
        long long totalBytes;

    public:
        SizeProcessor();
        virtual bool processNode(Node* node);
        long long getTotalBytes();
};

class TreeProcFolderInfo : public TreeProc
{
    public:
        TreeProcFolderInfo();
        virtual void proc(MegaClient*, Node*);
        virtual ~TreeProcFolderInfo() {}
        MegaFolderInfo *getResult();

    protected:
        int numFiles;
        int numFolders;
        int numVersions;
        long long currentSize;
        long long versionsSize;
};

//Thread safe request queue
class RequestQueue
{
    protected:
        std::deque<MegaRequestPrivate *> requests;
        MegaMutex mutex;

    public:
        RequestQueue();
        void push(MegaRequestPrivate *request);
        void push_front(MegaRequestPrivate *request);
        MegaRequestPrivate * pop();
        void removeListener(MegaRequestListener *listener);
#ifdef ENABLE_SYNC
        void removeListener(MegaSyncListener *listener);
#endif
        void removeListener(MegaBackupListener *listener);
};


//Thread safe transfer queue
class TransferQueue
{
    protected:
        std::deque<MegaTransferPrivate *> transfers;
        MegaMutex mutex;

    public:
        TransferQueue();
        void push(MegaTransferPrivate *transfer);
        void push_front(MegaTransferPrivate *transfer);
        MegaTransferPrivate * pop();
        void removeListener(MegaTransferListener *listener);
};

class MegaApiImpl : public MegaApp
{
    public:
        MegaApiImpl(MegaApi *api, const char *appKey, MegaGfxProcessor* processor, const char *basePath = NULL, const char *userAgent = NULL);
        MegaApiImpl(MegaApi *api, const char *appKey, const char *basePath = NULL, const char *userAgent = NULL);
        MegaApiImpl(MegaApi *api, const char *appKey, const char *basePath, const char *userAgent, int fseventsfd);
        virtual ~MegaApiImpl();

        //Multiple listener management.
        void addListener(MegaListener* listener);
        void addRequestListener(MegaRequestListener* listener);
        void addTransferListener(MegaTransferListener* listener);
        void addBackupListener(MegaBackupListener* listener);
        void addGlobalListener(MegaGlobalListener* listener);
#ifdef ENABLE_SYNC
        void addSyncListener(MegaSyncListener *listener);
        void removeSyncListener(MegaSyncListener *listener);
#endif
        void removeListener(MegaListener* listener);
        void removeRequestListener(MegaRequestListener* listener);
        void removeTransferListener(MegaTransferListener* listener);
        void removeBackupListener(MegaBackupListener* listener);
        void removeGlobalListener(MegaGlobalListener* listener);

        MegaRequest *getCurrentRequest();
        MegaTransfer *getCurrentTransfer();
        MegaError *getCurrentError();
        MegaNodeList *getCurrentNodes();
        MegaUserList *getCurrentUsers();

        //Utils
        long long getSDKtime();
        char *getStringHash(const char* base64pwkey, const char* inBuf);
        void getSessionTransferURL(const char *path, MegaRequestListener *listener);
        static MegaHandle base32ToHandle(const char* base32Handle);
        static handle base64ToHandle(const char* base64Handle);
        static handle base64ToUserHandle(const char* base64Handle);
        static char *handleToBase64(MegaHandle handle);
        static char *userHandleToBase64(MegaHandle handle);
        static const char* ebcEncryptKey(const char* encryptionKey, const char* plainKey);
        void retryPendingConnections(bool disconnect = false, bool includexfers = false, MegaRequestListener* listener = NULL);
        static void addEntropy(char* data, unsigned int size);
        static string userAttributeToString(int);
        static char userAttributeToScope(int);
        static void setStatsID(const char *id);

        bool serverSideRubbishBinAutopurgeEnabled();
        bool appleVoipPushEnabled();

        bool multiFactorAuthAvailable();
        void multiFactorAuthCheck(const char *email, MegaRequestListener *listener = NULL);
        void multiFactorAuthGetCode(MegaRequestListener *listener = NULL);
        void multiFactorAuthEnable(const char *pin, MegaRequestListener *listener = NULL);
        void multiFactorAuthDisable(const char *pin, MegaRequestListener *listener = NULL);
        void multiFactorAuthLogin(const char* email, const char* password, const char* pin, MegaRequestListener *listener = NULL);
        void multiFactorAuthChangePassword(const char *oldPassword, const char *newPassword, const char* pin, MegaRequestListener *listener = NULL);
        void multiFactorAuthChangeEmail(const char *email, const char* pin, MegaRequestListener *listener = NULL);
        void multiFactorAuthCancelAccount(const char* pin, MegaRequestListener *listener = NULL);

        void fetchTimeZone(MegaRequestListener *listener = NULL);

        //API requests
        void login(const char* email, const char* password, MegaRequestListener *listener = NULL);
        char *dumpSession();
        char *getSequenceNumber();
        char *dumpXMPPSession();
        char *getAccountAuth();
        void setAccountAuth(const char* auth);

        void fastLogin(const char* email, const char *stringHash, const char *base64pwkey, MegaRequestListener *listener = NULL);
        void fastLogin(const char* session, MegaRequestListener *listener = NULL);
        void killSession(MegaHandle sessionHandle, MegaRequestListener *listener = NULL);
        void getUserData(MegaRequestListener *listener = NULL);
        void getUserData(MegaUser *user, MegaRequestListener *listener = NULL);
        void getUserData(const char *user, MegaRequestListener *listener = NULL);
        void getAccountDetails(bool storage, bool transfer, bool pro, bool sessions, bool purchases, bool transactions, MegaRequestListener *listener = NULL);
        void queryTransferQuota(long long size, MegaRequestListener *listener = NULL);
        void createAccount(const char* email, const char* password, const char* name, MegaRequestListener *listener = NULL);
        void createAccount(const char* email, const char* password, const char* firstname, const char* lastname, MegaRequestListener *listener = NULL);
        void resumeCreateAccount(const char* sid, MegaRequestListener *listener = NULL);
        void sendSignupLink(const char* email, const char *name, const char *password, MegaRequestListener *listener = NULL);
        void fastSendSignupLink(const char *email, const char *base64pwkey, const char *name, MegaRequestListener *listener = NULL);
        void querySignupLink(const char* link, MegaRequestListener *listener = NULL);
        void confirmAccount(const char* link, const char *password, MegaRequestListener *listener = NULL);
        void fastConfirmAccount(const char* link, const char *base64pwkey, MegaRequestListener *listener = NULL);
        void resetPassword(const char *email, bool hasMasterKey, MegaRequestListener *listener = NULL);
        void queryRecoveryLink(const char *link, MegaRequestListener *listener = NULL);
        void confirmResetPasswordLink(const char *link, const char *newPwd, const char *masterKey = NULL, MegaRequestListener *listener = NULL);
        void cancelAccount(MegaRequestListener *listener = NULL);
        void confirmCancelAccount(const char *link, const char *pwd, MegaRequestListener *listener = NULL);
        void changeEmail(const char *email, MegaRequestListener *listener = NULL);
        void confirmChangeEmail(const char *link, const char *pwd, MegaRequestListener *listener = NULL);
        void setProxySettings(MegaProxy *proxySettings);
        MegaProxy *getAutoProxySettings();
        int isLoggedIn();
        void whyAmIBlocked(bool logout, MegaRequestListener *listener = NULL);
        char* getMyEmail();
        char* getMyUserHandle();
        MegaHandle getMyUserHandleBinary();
        MegaUser *getMyUser();
        char* getMyXMPPJid();
        bool isAchievementsEnabled();
        bool checkPassword(const char *password);
#ifdef ENABLE_CHAT
        char* getMyFingerprint();
#endif
        static void setLogLevel(int logLevel);
        static void addLoggerClass(MegaLogger *megaLogger);
        static void removeLoggerClass(MegaLogger *megaLogger);
        static void setLogToConsole(bool enable);
        static void log(int logLevel, const char* message, const char *filename = NULL, int line = -1);

        void createFolder(const char* name, MegaNode *parent, MegaRequestListener *listener = NULL);
        bool createLocalFolder(const char *path);
        void moveNode(MegaNode* node, MegaNode* newParent, MegaRequestListener *listener = NULL);
        void copyNode(MegaNode* node, MegaNode *newParent, MegaRequestListener *listener = NULL);
        void copyNode(MegaNode* node, MegaNode *newParent, const char* newName, MegaRequestListener *listener = NULL);
        void renameNode(MegaNode* node, const char* newName, MegaRequestListener *listener = NULL);
        void remove(MegaNode* node, bool keepversions = false, MegaRequestListener *listener = NULL);
        void removeVersions(MegaRequestListener *listener = NULL);
        void restoreVersion(MegaNode *version, MegaRequestListener *listener = NULL);
        void cleanRubbishBin(MegaRequestListener *listener = NULL);
        void sendFileToUser(MegaNode *node, MegaUser *user, MegaRequestListener *listener = NULL);
        void sendFileToUser(MegaNode *node, const char* email, MegaRequestListener *listener = NULL);
        void share(MegaNode *node, MegaUser* user, int level, MegaRequestListener *listener = NULL);
        void share(MegaNode* node, const char* email, int level, MegaRequestListener *listener = NULL);
        void loginToFolder(const char* megaFolderLink, MegaRequestListener *listener = NULL);
        void importFileLink(const char* megaFileLink, MegaNode* parent, MegaRequestListener *listener = NULL);
        void decryptPasswordProtectedLink(const char* link, const char* password, MegaRequestListener *listener = NULL);
        void encryptLinkWithPassword(const char* link, const char* password, MegaRequestListener *listener = NULL);
        void getPublicNode(const char* megaFileLink, MegaRequestListener *listener = NULL);
        void getThumbnail(MegaNode* node, const char *dstFilePath, MegaRequestListener *listener = NULL);
		void cancelGetThumbnail(MegaNode* node, MegaRequestListener *listener = NULL);
        void setThumbnail(MegaNode* node, const char *srcFilePath, MegaRequestListener *listener = NULL);
        void getPreview(MegaNode* node, const char *dstFilePath, MegaRequestListener *listener = NULL);
		void cancelGetPreview(MegaNode* node, MegaRequestListener *listener = NULL);
        void setPreview(MegaNode* node, const char *srcFilePath, MegaRequestListener *listener = NULL);
        void getUserAvatar(MegaUser* user, const char *dstFilePath, MegaRequestListener *listener = NULL);
        void setAvatar(const char *dstFilePath, MegaRequestListener *listener = NULL);
        void getUserAvatar(const char *email_or_handle, const char *dstFilePath, MegaRequestListener *listener = NULL);
        static char* getUserAvatarColor(MegaUser *user);
        static char *getUserAvatarColor(const char *userhandle);
        void getUserAttribute(MegaUser* user, int type, MegaRequestListener *listener = NULL);
        void getUserAttribute(const char* email_or_handle, int type, MegaRequestListener *listener = NULL);
        void getChatUserAttribute(const char* email_or_handle, int type, const char* ph, MegaRequestListener *listener = NULL);
        void getUserAttr(const char* email_or_handle, int type, const char *dstFilePath, int number = 0, MegaRequestListener *listener = NULL);
        void getChatUserAttr(const char* email_or_handle, int type, const char *dstFilePath, const char *ph = NULL, int number = 0, MegaRequestListener *listener = NULL);
        void setUserAttribute(int type, const char* value, MegaRequestListener *listener = NULL);
        void setUserAttribute(int type, const MegaStringMap* value, MegaRequestListener *listener = NULL);
        void enableRichPreviews(bool enable, MegaRequestListener *listener = NULL);
        void isRichPreviewsEnabled(MegaRequestListener *listener = NULL);
        void shouldShowRichLinkWarning(MegaRequestListener *listener = NULL);
        void setRichLinkWarningCounterValue(int value, MegaRequestListener *listener = NULL);
        void getRubbishBinAutopurgePeriod(MegaRequestListener *listener = NULL);
        void setRubbishBinAutopurgePeriod(int days, MegaRequestListener *listener = NULL);
        void getUserEmail(MegaHandle handle, MegaRequestListener *listener = NULL);
        void setCustomNodeAttribute(MegaNode *node, const char *attrName, const char *value, MegaRequestListener *listener = NULL);
        void setNodeDuration(MegaNode *node, int secs, MegaRequestListener *listener = NULL);
        void setNodeCoordinates(MegaNode *node, double latitude, double longitude, MegaRequestListener *listener = NULL);
        void exportNode(MegaNode *node, int64_t expireTime, MegaRequestListener *listener = NULL);
        void disableExport(MegaNode *node, MegaRequestListener *listener = NULL);
        void fetchNodes(MegaRequestListener *listener = NULL);
        void getPricing(MegaRequestListener *listener = NULL);
        void getPaymentId(handle productHandle, handle lastPublicHandle, MegaRequestListener *listener = NULL);
        void upgradeAccount(MegaHandle productHandle, int paymentMethod, MegaRequestListener *listener = NULL);
        void submitPurchaseReceipt(int gateway, const char* receipt, MegaHandle lastPublicHandle, MegaRequestListener *listener = NULL);
        void creditCardStore(const char* address1, const char* address2, const char* city,
                             const char* province, const char* country, const char *postalcode,
                             const char* firstname, const char* lastname, const char* creditcard,
                             const char* expire_month, const char* expire_year, const char* cv2,
                             MegaRequestListener *listener = NULL);

        void creditCardQuerySubscriptions(MegaRequestListener *listener = NULL);
        void creditCardCancelSubscriptions(const char* reason, MegaRequestListener *listener = NULL);
        void getPaymentMethods(MegaRequestListener *listener = NULL);

        char *exportMasterKey();
        void updatePwdReminderData(bool lastSuccess, bool lastSkipped, bool mkExported, bool dontShowAgain, bool lastLogin, MegaRequestListener *listener = NULL);

        void changePassword(const char *oldPassword, const char *newPassword, MegaRequestListener *listener = NULL);
        void inviteContact(const char* email, const char* message, int action, MegaHandle contactLink, MegaRequestListener* listener = NULL);
        void replyContactRequest(MegaContactRequest *request, int action, MegaRequestListener* listener = NULL);
        void respondContactRequest();

        void removeContact(MegaUser *user, MegaRequestListener* listener=NULL);
        void logout(MegaRequestListener *listener = NULL);
        void localLogout(MegaRequestListener *listener = NULL);
        void invalidateCache();
        int getPasswordStrength(const char *password);
        void submitFeedback(int rating, const char *comment, MegaRequestListener *listener = NULL);
        void reportEvent(const char *details = NULL, MegaRequestListener *listener = NULL);
        void sendEvent(int eventType, const char* message, MegaRequestListener *listener = NULL);

        void useHttpsOnly(bool httpsOnly, MegaRequestListener *listener = NULL);
        bool usingHttpsOnly();

        //Backups
        MegaStringList *getBackupFolders(int backuptag);
        void setBackup(const char* localPath, MegaNode *parent, bool attendPastBackups, int64_t period, string periodstring, int numBackups, MegaRequestListener *listener=NULL);
        void removeBackup(int tag, MegaRequestListener *listener=NULL);
        void abortCurrentBackup(int tag, MegaRequestListener *listener=NULL);

        //Timer
        void startTimer( int64_t period, MegaRequestListener *listener=NULL);

        //Transfers
        void startUpload(const char* localPath, MegaNode *parent, MegaTransferListener *listener=NULL);
        void startUpload(const char* localPath, MegaNode *parent, int64_t mtime, MegaTransferListener *listener=NULL);
        void startUpload(const char* localPath, MegaNode* parent, const char* fileName, MegaTransferListener *listener = NULL);
        void startUpload(bool startFirst, const char* localPath, MegaNode* parent, const char* fileName,  int64_t mtime, int folderTransferTag = 0, const char *appData = NULL, bool isSourceFileTemporary = false, MegaTransferListener *listener = NULL);
        void startDownload(MegaNode* node, const char* localPath, MegaTransferListener *listener = NULL);
        void startDownload(bool startFirst, MegaNode *node, const char* target, int folderTransferTag, const char *appData, MegaTransferListener *listener);
        void startStreaming(MegaNode* node, m_off_t startPos, m_off_t size, MegaTransferListener *listener);
        void retryTransfer(MegaTransfer *transfer, MegaTransferListener *listener = NULL);
        void cancelTransfer(MegaTransfer *transfer, MegaRequestListener *listener=NULL);
        void cancelTransferByTag(int transferTag, MegaRequestListener *listener = NULL);
        void cancelTransfers(int direction, MegaRequestListener *listener=NULL);
        void pauseTransfers(bool pause, int direction, MegaRequestListener* listener=NULL);
        void pauseTransfer(int transferTag, bool pause, MegaRequestListener* listener = NULL);
        void moveTransferUp(int transferTag, MegaRequestListener *listener = NULL);
        void moveTransferDown(int transferTag, MegaRequestListener *listener = NULL);
        void moveTransferToFirst(int transferTag, MegaRequestListener *listener = NULL);
        void moveTransferToLast(int transferTag, MegaRequestListener *listener = NULL);
        void moveTransferBefore(int transferTag, int prevTransferTag, MegaRequestListener *listener = NULL);
        void enableTransferResumption(const char* loggedOutId);
        void disableTransferResumption(const char* loggedOutId);
        bool areTransfersPaused(int direction);
        void setUploadLimit(int bpslimit);
        void setMaxConnections(int direction, int connections, MegaRequestListener* listener = NULL);
        void setDownloadMethod(int method);
        void setUploadMethod(int method);
        bool setMaxDownloadSpeed(m_off_t bpslimit);
        bool setMaxUploadSpeed(m_off_t bpslimit);
        int getMaxDownloadSpeed();
        int getMaxUploadSpeed();
        int getCurrentDownloadSpeed();
        int getCurrentUploadSpeed();
        int getCurrentSpeed(int type);
        int getDownloadMethod();
        int getUploadMethod();
        MegaTransferData *getTransferData(MegaTransferListener *listener = NULL);
        MegaTransfer *getFirstTransfer(int type);
        void notifyTransfer(int transferTag, MegaTransferListener *listener = NULL);
        MegaTransferList *getTransfers();
        MegaTransferList *getStreamingTransfers();
        MegaTransfer* getTransferByTag(int transferTag);
        MegaTransferList *getTransfers(int type);
        MegaTransferList *getChildTransfers(int transferTag);
        MegaTransferList *getTansfersByFolderTag(int folderTransferTag);


#ifdef ENABLE_SYNC
        //Sync
        int syncPathState(string *path);
        MegaNode *getSyncedNode(string *path);
        void syncFolder(const char *localFolder, MegaNode *megaFolder, MegaRegExp *regExp = NULL, MegaRequestListener* listener = NULL);
        void resumeSync(const char *localFolder, long long localfp, MegaNode *megaFolder, MegaRegExp *regExp = NULL, MegaRequestListener *listener = NULL);
        void removeSync(handle nodehandle, MegaRequestListener *listener=NULL);
        void disableSync(handle nodehandle, MegaRequestListener *listener=NULL);
        int getNumActiveSyncs();
        void stopSyncs(MegaRequestListener *listener=NULL);
        bool isSynced(MegaNode *n);
        void setExcludedNames(vector<string> *excludedNames);
        void setExcludedPaths(vector<string> *excludedPaths);
        void setExclusionLowerSizeLimit(long long limit);
        void setExclusionUpperSizeLimit(long long limit);
        bool moveToLocalDebris(const char *path);
        string getLocalPath(MegaNode *node);
        long long getNumLocalNodes();
        bool isSyncable(const char *path, long long size);
        bool is_syncable(Sync*, const char*, string*);
        bool is_syncable(long long size);
        int isNodeSyncable(MegaNode *megaNode);
        bool isIndexing();
        MegaSync *getSyncByTag(int tag);
        MegaSync *getSyncByNode(MegaNode *node);
        MegaSync *getSyncByPath(const char * localPath);
        char *getBlockedPath();
        void setExcludedRegularExpressions(MegaSync *sync, MegaRegExp *regExp);
#endif

        MegaBackup *getBackupByTag(int tag);
        MegaBackup *getBackupByNode(MegaNode *node);
        MegaBackup *getBackupByPath(const char * localPath);

        void update();
        int isWaiting();
        int areServersBusy();

        //Statistics
        int getNumPendingUploads();
        int getNumPendingDownloads();
        int getTotalUploads();
        int getTotalDownloads();
        void resetTotalDownloads();
        void resetTotalUploads();
        void updateStats();
        long long getNumNodes();
        long long getTotalDownloadedBytes();
        long long getTotalUploadedBytes();
        long long getTotalDownloadBytes();
        long long getTotalUploadBytes();

        //Filesystem
		int getNumChildren(MegaNode* parent);
		int getNumChildFiles(MegaNode* parent);
		int getNumChildFolders(MegaNode* parent);
        MegaNodeList* getChildren(MegaNode *parent, int order=1);
        MegaNodeList* getVersions(MegaNode *node);
        int getNumVersions(MegaNode *node);
        bool hasVersions(MegaNode *node);
        void getFolderInfo(MegaNode *node, MegaRequestListener *listener);
        MegaChildrenLists* getFileFolderChildren(MegaNode *parent, int order=1);
        bool hasChildren(MegaNode *parent);
        int getIndex(MegaNode* node, int order=1);
        MegaNode *getChildNode(MegaNode *parent, const char* name);
        MegaNode *getParentNode(MegaNode *node);
        char *getNodePath(MegaNode *node);
        MegaNode *getNodeByPath(const char *path, MegaNode *n = NULL);
        MegaNode *getNodeByHandle(handle handler);
        MegaContactRequest *getContactRequestByHandle(MegaHandle handle);
        MegaUserList* getContacts();
        MegaUser* getContact(const char* uid);
        MegaUserAlertList* getUserAlerts();
        int getNumUnreadUserAlerts();
        MegaNodeList *getInShares(MegaUser* user);
        MegaNodeList *getInShares();
        MegaShareList *getInSharesList();
        MegaUser *getUserFromInShare(MegaNode *node);
        bool isPendingShare(MegaNode *node);
        MegaShareList *getOutShares();
        MegaShareList *getOutShares(MegaNode *node);
        MegaShareList *getPendingOutShares();
        MegaShareList *getPendingOutShares(MegaNode *megaNode);
        MegaNodeList *getPublicLinks();
        MegaContactRequestList *getIncomingContactRequests();
        MegaContactRequestList *getOutgoingContactRequests();

        int getAccess(MegaNode* node);
        long long getSize(MegaNode *node);
        static void removeRecursively(const char *path);

        //Fingerprint
        char *getFingerprint(const char *filePath);
        char *getFingerprint(MegaNode *node);
        char *getFingerprint(MegaInputStream *inputStream, int64_t mtime);
        MegaNode *getNodeByFingerprint(const char* fingerprint);
        MegaNodeList *getNodesByFingerprint(const char* fingerprint);
        MegaNode *getExportableNodeByFingerprint(const char *fingerprint, const char *name = NULL);
        MegaNode *getNodeByFingerprint(const char *fingerprint, MegaNode* parent);
        bool hasFingerprint(const char* fingerprint);

        //CRC
        char *getCRC(const char *filePath);
        char *getCRCFromFingerprint(const char *fingerprint);
        char *getCRC(MegaNode *node);
        MegaNode* getNodeByCRC(const char *crc, MegaNode* parent);

        //Permissions
        MegaError checkAccess(MegaNode* node, int level);
        MegaError checkMove(MegaNode* node, MegaNode* target);

        bool isFilesystemAvailable();
        MegaNode *getRootNode();
        MegaNode* getInboxNode();
        MegaNode *getRubbishNode();
        MegaNode *getRootNode(MegaNode *node);
        bool isInRootnode(MegaNode *node, int index);

        void setDefaultFilePermissions(int permissions);
        int getDefaultFilePermissions();
        void setDefaultFolderPermissions(int permissions);
        int getDefaultFolderPermissions();

        long long getBandwidthOverquotaDelay();

        MegaNodeList* search(MegaNode* node, const char* searchString, bool recursive = 1, int order = MegaApi::ORDER_NONE);
        bool processMegaTree(MegaNode* node, MegaTreeProcessor* processor, bool recursive = 1);
        MegaNodeList* search(const char* searchString, int order = MegaApi::ORDER_NONE);

        MegaNode *createForeignFileNode(MegaHandle handle, const char *key, const char *name, m_off_t size, m_off_t mtime,
                                       MegaHandle parentHandle, const char *privateauth, const char *publicauth);
        MegaNode *createForeignFolderNode(MegaHandle handle, const char *name, MegaHandle parentHandle,
                                         const char *privateauth, const char *publicauth);

        MegaNode *authorizeNode(MegaNode *node);
        void authorizeMegaNodePrivate(MegaNodePrivate *node);
        MegaNode *authorizeChatNode(MegaNode *node, const char *cauth);

        const char *getVersion();
        char *getOperatingSystemVersion();
        void getLastAvailableVersion(const char *appKey, MegaRequestListener *listener = NULL);
        void getLocalSSLCertificate(MegaRequestListener *listener = NULL);
        void queryDNS(const char *hostname, MegaRequestListener *listener = NULL);
        void queryGeLB(const char *service, int timeoutds, int maxretries, MegaRequestListener *listener = NULL);
        void downloadFile(const char *url, const char *dstpath, MegaRequestListener *listener = NULL);
        const char *getUserAgent();
        const char *getBasePath();

        void contactLinkCreate(bool renew = false, MegaRequestListener *listener = NULL);
        void contactLinkQuery(MegaHandle handle, MegaRequestListener *listener = NULL);
        void contactLinkDelete(MegaHandle handle, MegaRequestListener *listener = NULL);

        void keepMeAlive(int type, bool enable, MegaRequestListener *listener = NULL);
        void acknowledgeUserAlerts(MegaRequestListener *listener = NULL);

        void getPSA(MegaRequestListener *listener = NULL);
        void setPSA(int id, MegaRequestListener *listener = NULL);

        void disableGfxFeatures(bool disable);
        bool areGfxFeaturesDisabled();

        void changeApiUrl(const char *apiURL, bool disablepkp = false);

        bool setLanguage(const char* languageCode);
        void setLanguagePreference(const char* languageCode, MegaRequestListener *listener = NULL);
        void getLanguagePreference(MegaRequestListener *listener = NULL);
        bool getLanguageCode(const char* languageCode, std::string* code);

        void setFileVersionsOption(bool disable, MegaRequestListener *listener = NULL);
        void getFileVersionsOption(MegaRequestListener *listener = NULL);

        void setContactLinksOption(bool disable, MegaRequestListener *listener = NULL);
        void getContactLinksOption(MegaRequestListener *listener = NULL);

        void retrySSLerrors(bool enable);
        void setPublicKeyPinning(bool enable);
        void pauseActionPackets();
        void resumeActionPackets();

        static bool nodeComparatorDefaultASC  (Node *i, Node *j);
        static bool nodeComparatorDefaultDESC (Node *i, Node *j);
        static bool nodeComparatorSizeASC  (Node *i, Node *j);
        static bool nodeComparatorSizeDESC (Node *i, Node *j);
        static bool nodeComparatorCreationASC  (Node *i, Node *j);
        static bool nodeComparatorCreationDESC  (Node *i, Node *j);
        static bool nodeComparatorModificationASC  (Node *i, Node *j);
        static bool nodeComparatorModificationDESC  (Node *i, Node *j);
        static bool nodeComparatorAlphabeticalASC  (Node *i, Node *j);
        static bool nodeComparatorAlphabeticalDESC  (Node *i, Node *j);
        static bool userComparatorDefaultASC (User *i, User *j);

        char* escapeFsIncompatible(const char *filename);
        char* unescapeFsIncompatible(const char* name);

        bool createThumbnail(const char* imagePath, const char *dstPath);
        bool createPreview(const char* imagePath, const char *dstPath);
        bool createAvatar(const char* imagePath, const char *dstPath);

        bool isOnline();

#ifdef HAVE_LIBUV
        // start/stop
        bool httpServerStart(bool localOnly = true, int port = 4443, bool useTLS = false, const char *certificatepath = NULL, const char *keypath = NULL);
        void httpServerStop();
        int httpServerIsRunning();

        // management
        char *httpServerGetLocalLink(MegaNode *node);
        char *httpServerGetLocalWebDavLink(MegaNode *node);
        MegaStringList *httpServerGetWebDavLinks();
        MegaNodeList *httpServerGetWebDavAllowedNodes();
        void httpServerRemoveWebDavAllowedNode(MegaHandle handle);
        void httpServerRemoveWebDavAllowedNodes();
        void httpServerSetMaxBufferSize(int bufferSize);
        int httpServerGetMaxBufferSize();
        void httpServerSetMaxOutputSize(int outputSize);
        int httpServerGetMaxOutputSize();

        // permissions
        void httpServerEnableFileServer(bool enable);
        bool httpServerIsFileServerEnabled();
        void httpServerEnableFolderServer(bool enable);
        bool httpServerIsFolderServerEnabled();
        bool httpServerIsOfflineAttributeEnabled();
        void httpServerSetRestrictedMode(int mode);
        int httpServerGetRestrictedMode();
        bool httpServerIsLocalOnly();
        void httpServerEnableOfflineAttribute(bool enable);
        void httpServerEnableSubtitlesSupport(bool enable);
        bool httpServerIsSubtitlesSupportEnabled();

        void httpServerAddListener(MegaTransferListener *listener);
        void httpServerRemoveListener(MegaTransferListener *listener);

        void fireOnStreamingStart(MegaTransferPrivate *transfer);
        void fireOnStreamingTemporaryError(MegaTransferPrivate *transfer, MegaError e);
        void fireOnStreamingFinish(MegaTransferPrivate *transfer, MegaError e);

        //FTP
        bool ftpServerStart(bool localOnly = true, int port = 4990, int dataportBegin = 1500, int dataPortEnd = 1600, bool useTLS = false, const char *certificatepath = NULL, const char *keypath = NULL);
        void ftpServerStop();
        int ftpServerIsRunning();

        // management
        char *ftpServerGetLocalLink(MegaNode *node);
        MegaStringList *ftpServerGetLinks();
        MegaNodeList *ftpServerGetAllowedNodes();
        void ftpServerRemoveAllowedNode(MegaHandle handle);
        void ftpServerRemoveAllowedNodes();
        void ftpServerSetMaxBufferSize(int bufferSize);
        int ftpServerGetMaxBufferSize();
        void ftpServerSetMaxOutputSize(int outputSize);
        int ftpServerGetMaxOutputSize();

        // permissions
        void ftpServerSetRestrictedMode(int mode);
        int ftpServerGetRestrictedMode();
        bool ftpServerIsLocalOnly();

        void ftpServerAddListener(MegaTransferListener *listener);
        void ftpServerRemoveListener(MegaTransferListener *listener);

        void fireOnFtpStreamingStart(MegaTransferPrivate *transfer);
        void fireOnFtpStreamingTemporaryError(MegaTransferPrivate *transfer, MegaError e);
        void fireOnFtpStreamingFinish(MegaTransferPrivate *transfer, MegaError e);

#endif

#ifdef ENABLE_CHAT
        void createChat(bool group, bool publicchat, MegaTextChatPeerList *peers, const MegaStringMap *userKeyMap = NULL, const char *title = NULL, MegaRequestListener *listener = NULL);
        void inviteToChat(MegaHandle chatid, MegaHandle uh, int privilege, bool openMode, const char *unifiedKey = NULL, const char *title = NULL, MegaRequestListener *listener = NULL);
        void removeFromChat(MegaHandle chatid, MegaHandle uh = INVALID_HANDLE, MegaRequestListener *listener = NULL);
        void getUrlChat(MegaHandle chatid, MegaRequestListener *listener = NULL);
        void grantAccessInChat(MegaHandle chatid, MegaNode *n, MegaHandle uh,  MegaRequestListener *listener = NULL);
        void removeAccessInChat(MegaHandle chatid, MegaNode *n, MegaHandle uh,  MegaRequestListener *listener = NULL);
        void updateChatPermissions(MegaHandle chatid, MegaHandle uh, int privilege, MegaRequestListener *listener = NULL);
        void truncateChat(MegaHandle chatid, MegaHandle messageid, MegaRequestListener *listener = NULL);
        void setChatTitle(MegaHandle chatid, const char *title, MegaRequestListener *listener = NULL);
        void setChatUnifiedKey(MegaHandle chatid, const char *unifiedKey, MegaRequestListener *listener = NULL);
        void getChatPresenceURL(MegaRequestListener *listener = NULL);
        void registerPushNotification(int deviceType, const char *token, MegaRequestListener *listener = NULL);
        void sendChatStats(const char *data, int port, MegaRequestListener *listener = NULL);
        void sendChatLogs(const char *data, const char *aid, MegaRequestListener *listener = NULL);
        MegaTextChatList *getChatList();
        MegaHandleList *getAttachmentAccess(MegaHandle chatid, MegaHandle h);
        bool hasAccessToAttachment(MegaHandle chatid, MegaHandle h, MegaHandle uh);
        const char* getFileAttribute(MegaHandle h);
        void archiveChat(MegaHandle chatid, int archive, MegaRequestListener *listener = NULL);
        void requestRichPreview(const char *url, MegaRequestListener *listener = NULL);
        void chatLinkHandle(MegaHandle chatid, bool del, bool createifmissing, MegaRequestListener *listener = NULL);
        void getChatLinkURL(MegaHandle publichandle, MegaRequestListener *listener = NULL);
        void chatLinkClose(MegaHandle chatid, const char *title, MegaRequestListener *listener = NULL);
        void chatLinkJoin(MegaHandle publichandle, const char *unifiedkey, MegaRequestListener *listener = NULL);
#endif

        void getAccountAchievements(MegaRequestListener *listener = NULL);
        void getMegaAchievements(MegaRequestListener *listener = NULL);

        void fireOnTransferStart(MegaTransferPrivate *transfer);
        void fireOnTransferFinish(MegaTransferPrivate *transfer, MegaError e);
        void fireOnTransferUpdate(MegaTransferPrivate *transfer);
        void fireOnTransferTemporaryError(MegaTransferPrivate *transfer, MegaError e);
        map<int, MegaTransferPrivate *> transferMap;

        MegaClient *getMegaClient();
        static FileFingerprint *getFileFingerprintInternal(const char *fingerprint);

        // You take the ownership of the returned value of both functiions
        // It can be NULL if the input parameters are invalid
        static char* getMegaFingerprintFromSdkFingerprint(const char* sdkFingerprint);
        static char* getSdkFingerprintFromMegaFingerprint(const char *megaFingerprint, m_off_t size);

        error processAbortBackupRequest(MegaRequestPrivate *request, error e);
        void fireOnBackupStateChanged(MegaBackupController *backup);
        void fireOnBackupStart(MegaBackupController *backup);
        void fireOnBackupFinish(MegaBackupController *backup, MegaError e);
        void fireOnBackupUpdate(MegaBackupController *backup);
        void fireOnBackupTemporaryError(MegaBackupController *backup, MegaError e);

protected:
        static const unsigned int MAX_SESSION_LENGTH;

        void init(MegaApi *api, const char *appKey, MegaGfxProcessor* processor, const char *basePath = NULL, const char *userAgent = NULL, int fseventsfd = -1);

        static void *threadEntryPoint(void *param);
        static ExternalLogger externalLogger;

        MegaTransferPrivate* getMegaTransferPrivate(int tag);

        void fireOnRequestStart(MegaRequestPrivate *request);
        void fireOnRequestFinish(MegaRequestPrivate *request, MegaError e);
        void fireOnRequestUpdate(MegaRequestPrivate *request);
        void fireOnRequestTemporaryError(MegaRequestPrivate *request, MegaError e);
        bool fireOnTransferData(MegaTransferPrivate *transfer);
        void fireOnUsersUpdate(MegaUserList *users);
        void fireOnUserAlertsUpdate(MegaUserAlertList *alerts);
        void fireOnNodesUpdate(MegaNodeList *nodes);
        void fireOnAccountUpdate();
        void fireOnContactRequestsUpdate(MegaContactRequestList *requests);
        void fireOnReloadNeeded();
        void fireOnEvent(MegaEventPrivate *event);

#ifdef ENABLE_SYNC
        void fireOnGlobalSyncStateChanged();
        void fireOnSyncStateChanged(MegaSyncPrivate *sync);
        void fireOnSyncEvent(MegaSyncPrivate *sync, MegaSyncEvent *event);
        void fireOnFileSyncStateChanged(MegaSyncPrivate *sync, string *localPath, int newState);
#endif

#ifdef ENABLE_CHAT
        void fireOnChatsUpdate(MegaTextChatList *chats);
#endif

        void processTransferPrepare(Transfer *t, MegaTransferPrivate *transfer);
        void processTransferUpdate(Transfer *tr, MegaTransferPrivate *transfer);
        void processTransferComplete(Transfer *tr, MegaTransferPrivate *transfer);
        void processTransferFailed(Transfer *tr, MegaTransferPrivate *transfer, error error, dstime timeleft);
        void processTransferRemoved(Transfer *tr, MegaTransferPrivate *transfer, error e);

        MegaApi *api;
        MegaThread thread;
        MegaClient *client;
        MegaHttpIO *httpio;
        MegaWaiter *waiter;
        MegaFileSystemAccess *fsAccess;
        MegaDbAccess *dbAccess;
        GfxProc *gfxAccess;
        string basePath;
        bool nocache;

#ifdef HAVE_LIBUV
        MegaHTTPServer *httpServer;
        int httpServerMaxBufferSize;
        int httpServerMaxOutputSize;
        bool httpServerEnableFiles;
        bool httpServerEnableFolders;
        bool httpServerOfflineAttributeEnabled;
        int httpServerRestrictedMode;
        bool httpServerSubtitlesSupportEnabled;
        set<MegaTransferListener *> httpServerListeners;

        MegaFTPServer *ftpServer;
        int ftpServerMaxBufferSize;
        int ftpServerMaxOutputSize;
        int ftpServerRestrictedMode;
        set<MegaTransferListener *> ftpServerListeners;

#endif
		
        map<int, MegaBackupController *> backupsMap;

        RequestQueue requestQueue;
        TransferQueue transferQueue;
        map<int, MegaRequestPrivate *> requestMap;

#ifdef ENABLE_SYNC
        map<int, MegaSyncPrivate *> syncMap;
#endif

        int pendingUploads;
        int pendingDownloads;
        int totalUploads;
        int totalDownloads;
        long long totalDownloadedBytes;
        long long totalUploadedBytes;
        long long totalDownloadBytes;
        long long totalUploadBytes;
        long long notificationNumber;
        set<MegaRequestListener *> requestListeners;
        set<MegaTransferListener *> transferListeners;
        set<MegaBackupListener *> backupListeners;

#ifdef ENABLE_SYNC
        set<MegaSyncListener *> syncListeners;
#endif

        set<MegaGlobalListener *> globalListeners;
        set<MegaListener *> listeners;
        retryreason_t waitingRequest;
        vector<string> excludedNames;
        vector<string> excludedPaths;
        long long syncLowerSizeLimit;
        long long syncUpperSizeLimit;
        MegaMutex sdkMutex;
        MegaTransferPrivate *currentTransfer;
        MegaRequestPrivate *activeRequest;
        MegaTransferPrivate *activeTransfer;
        MegaError *activeError;
        MegaNodeList *activeNodes;
        MegaUserList *activeUsers;
        MegaUserAlertList *activeUserAlerts;
        MegaContactRequestList *activeContactRequests;
        string appKey;

        int threadExit;
        void loop();

        int maxRetries;

        // a request-level error occurred
        virtual void request_error(error);
        virtual void request_response_progress(m_off_t, m_off_t);

        // login result
        virtual void prelogin_result(int, string*, string*, error);
        virtual void login_result(error);
        virtual void logout_result(error);
        virtual void userdata_result(string*, string*, string*, handle, error);
        virtual void pubkey_result(User *);

        // ephemeral session creation/resumption result
        virtual void ephemeral_result(error);
        virtual void ephemeral_result(handle, const byte*);

        // check the reason of being blocked
        virtual void whyamiblocked_result(int);

        // contact link management
        virtual void contactlinkcreate_result(error, handle);
        virtual void contactlinkquery_result(error, handle, string*, string*, string*, string*);
        virtual void contactlinkdelete_result(error);

        // multi-factor authentication
        virtual void multifactorauthsetup_result(string*, error);
        virtual void multifactorauthcheck_result(int);
        virtual void multifactorauthdisable_result(error);

        // fetch time zone
        virtual void fetchtimezone_result(error, vector<string>*, vector<int>*, int);

        // keep me alive feature
        virtual void keepmealive_result(error);
        virtual void acknowledgeuseralerts_result(error);

        // get the current PSA
        virtual void getpsa_result (error, int, string*, string*, string*, string*, string*);

        // account creation
        virtual void sendsignuplink_result(error);
        virtual void querysignuplink_result(error);
        virtual void querysignuplink_result(handle, const char*, const char*, const byte*, const byte*, const byte*, size_t);
        virtual void confirmsignuplink_result(error);
        virtual void confirmsignuplink2_result(handle, const char*, const char*, error);
        virtual void setkeypair_result(error);

        // account credentials, properties and history
        virtual void account_details(AccountDetails*,  bool, bool, bool, bool, bool, bool);
        virtual void account_details(AccountDetails*, error);
        virtual void querytransferquota_result(int);

        virtual void setattr_result(handle, error);
        virtual void rename_result(handle, error);
        virtual void unlink_result(handle, error);
        virtual void unlinkversions_result(error);
        virtual void nodes_updated(Node**, int);
        virtual void users_updated(User**, int);
        virtual void useralerts_updated(UserAlert::Base**, int);
        virtual void account_updated();
        virtual void pcrs_updated(PendingContactRequest**, int);

        // password change result
        virtual void changepw_result(error);

        // user attribute update notification
        virtual void userattr_update(User*, int, const char*);


        virtual void fetchnodes_result(error);
        virtual void putnodes_result(error, targettype_t, NewNode*);

        // share update result
        virtual void share_result(error);
        virtual void share_result(int, error);

        // contact request results
        void setpcr_result(handle, error, opcactions_t);
        void updatepcr_result(error, ipcactions_t);

        // file attribute fetch result
        virtual void fa_complete(handle, fatype, const char*, uint32_t);
        virtual int fa_failed(handle, fatype, int, error);

        // file attribute modification result
        virtual void putfa_result(handle, fatype, error);
        virtual void putfa_result(handle, fatype, const char*);

        // purchase transactions
        virtual void enumeratequotaitems_result(handle product, unsigned prolevel, unsigned gbstorage, unsigned gbtransfer,
                                                unsigned months, unsigned amount, const char* currency, const char* description, const char* iosid, const char* androidid);
        virtual void enumeratequotaitems_result(error e);
        virtual void additem_result(error);
        virtual void checkout_result(const char*, error);
        virtual void submitpurchasereceipt_result(error);
        virtual void creditcardstore_result(error);
        virtual void creditcardquerysubscriptions_result(int, error);
        virtual void creditcardcancelsubscriptions_result(error);
        virtual void getpaymentmethods_result(int, error);
        virtual void copysession_result(string*, error);

        virtual void userfeedbackstore_result(error);
        virtual void sendevent_result(error);

        virtual void checkfile_result(handle h, error e);
        virtual void checkfile_result(handle h, error e, byte* filekey, m_off_t size, m_time_t ts, m_time_t tm, string* filename, string* fingerprint, string* fileattrstring);

        // user invites/attributes
        virtual void removecontact_result(error);
        virtual void putua_result(error);
        virtual void getua_result(error);
        virtual void getua_result(byte*, unsigned);
        virtual void getua_result(TLVstore *);
#ifdef DEBUG
        virtual void delua_result(error);
#endif

        virtual void getuseremail_result(string *, error);

        // file node export result
        virtual void exportnode_result(error);
        virtual void exportnode_result(handle, handle);

        // exported link access result
        virtual void openfilelink_result(error);
        virtual void openfilelink_result(handle, const byte*, m_off_t, string*, string*, int);

        // global transfer queue updates (separate signaling towards the queued objects)
        virtual void file_added(File*);
        virtual void file_removed(File*, error e);
        virtual void file_complete(File*);
        virtual File* file_resume(string*, direction_t *type);

        virtual void transfer_prepare(Transfer*);
        virtual void transfer_failed(Transfer*, error error, dstime timeleft);
        virtual void transfer_update(Transfer*);

        virtual dstime pread_failure(error, int, void*, dstime);
        virtual bool pread_data(byte*, m_off_t, m_off_t, m_off_t, m_off_t, void*);

        virtual void reportevent_result(error);
        virtual void sessions_killed(handle sessionid, error e);

        virtual void cleanrubbishbin_result(error);

        virtual void getrecoverylink_result(error);
        virtual void queryrecoverylink_result(error);
        virtual void queryrecoverylink_result(int type, const char *email, const char *ip, time_t ts, handle uh, const vector<string> *emails);
        virtual void getprivatekey_result(error, const byte *privk = NULL, const size_t len_privk = 0);
        virtual void confirmrecoverylink_result(error);
        virtual void confirmcancellink_result(error);
        virtual void getemaillink_result(error);
        virtual void confirmemaillink_result(error);
        virtual void getversion_result(int, const char*, error);
        virtual void getlocalsslcertificate_result(m_time_t, string *certdata, error);
        virtual void getmegaachievements_result(AchievementsDetails*, error);
        virtual void getwelcomepdf_result(handle, string*, error);

#ifdef ENABLE_CHAT
        // chat-related commandsresult
        virtual void chatcreate_result(TextChat *, error);
        virtual void chatinvite_result(error);
        virtual void chatremove_result(error);
        virtual void chaturl_result(error);
        virtual void chaturl_result(string*, error);
        virtual void chatgrantaccess_result(error);
        virtual void chatremoveaccess_result(error);
        virtual void chatupdatepermissions_result(error);
        virtual void chattruncate_result(error);
        virtual void chatsettitle_result(error);
        virtual void chatpresenceurl_result(string*, error);
        virtual void registerpushnotification_result(error);
        virtual void archivechat_result(error);

        virtual void chats_updated(textchat_map *, int);
        virtual void richlinkrequest_result(string*, error);
        virtual void chatlink_result(handle, error);
        virtual void chatlinkurl_result(handle, int, string*, string*, int, error);
        virtual void chatlinkclose_result(error);
        virtual void chatlinkjoin_result(error);
#endif

#ifdef ENABLE_SYNC
        // sync status updates and events
        virtual void syncupdate_state(Sync*, syncstate_t);
        virtual void syncupdate_scanning(bool scanning);
        virtual void syncupdate_local_folder_addition(Sync* sync, LocalNode *localNode, const char *path);
        virtual void syncupdate_local_folder_deletion(Sync* sync, LocalNode *localNode);
        virtual void syncupdate_local_file_addition(Sync* sync, LocalNode* localNode, const char *path);
        virtual void syncupdate_local_file_deletion(Sync* sync, LocalNode* localNode);
        virtual void syncupdate_local_file_change(Sync* sync, LocalNode* localNode, const char *path);
        virtual void syncupdate_local_move(Sync* sync, LocalNode* localNode, const char* path);
        virtual void syncupdate_get(Sync* sync, Node *node, const char* path);
        virtual void syncupdate_put(Sync* sync, LocalNode *localNode, const char*);
        virtual void syncupdate_remote_file_addition(Sync *sync, Node* n);
        virtual void syncupdate_remote_file_deletion(Sync *sync, Node* n);
        virtual void syncupdate_remote_folder_addition(Sync *sync, Node* n);
        virtual void syncupdate_remote_folder_deletion(Sync* sync, Node* n);
        virtual void syncupdate_remote_copy(Sync*, const char*);
        virtual void syncupdate_remote_move(Sync *sync, Node *n, Node* prevparent);
        virtual void syncupdate_remote_rename(Sync*sync, Node* n, const char* prevname);
        virtual void syncupdate_treestate(LocalNode*);
        virtual bool sync_syncable(Sync *, const char*, string *, Node *);
        virtual bool sync_syncable(Sync *, const char*, string *);
        virtual void syncupdate_local_lockretry(bool);
#endif

protected:
        // suggest reload due to possible race condition with other clients
        virtual void reload(const char*);

        // wipe all users, nodes and shares
        virtual void clearing();

        // failed request retry notification
        virtual void notify_retry(dstime, retryreason_t);

        // notify about db commit
        virtual void notify_dbcommit();

        // notify about an automatic change to HTTPS
        virtual void notify_change_to_https();

        // notify about account confirmation
        virtual void notify_confirmation(const char*);

        // network layer disconnected
        virtual void notify_disconnect();

        // notify about a finished HTTP request
        virtual void http_result(error, int, byte *, int);

        // notify about a finished timer
        virtual void timer_result(error);

        void sendPendingRequests();
        void sendPendingTransfers();
        void updateBackups();
        char *stringToArray(string &buffer);

        //Internal
        Node* getNodeByFingerprintInternal(const char *fingerprint);
        Node *getNodeByFingerprintInternal(const char *fingerprint, Node *parent);

        bool processTree(Node* node, TreeProcessor* processor, bool recursive = 1);
        MegaNodeList* search(Node* node, const char* searchString, bool recursive = 1);
        void getNodeAttribute(MegaNode* node, int type, const char *dstFilePath, MegaRequestListener *listener = NULL);
		void cancelGetNodeAttribute(MegaNode *node, int type, MegaRequestListener *listener = NULL);
        void setNodeAttribute(MegaNode* node, int type, const char *srcFilePath, MegaRequestListener *listener = NULL);
        void setUserAttr(int type, const char *value, MegaRequestListener *listener = NULL);
        static char *getAvatarColor(handle userhandle);
};

class MegaHashSignatureImpl
{
	public:
		MegaHashSignatureImpl(const char *base64Key);
		~MegaHashSignatureImpl();
		void init();
		void add(const char *data, unsigned size);
        bool checkSignature(const char *base64Signature);

	protected:    
		HashSignature *hashSignature;
		AsymmCipher* asymmCypher;
};

class ExternalInputStream : public InputStreamAccess
{
    MegaInputStream *inputStream;

public:
    ExternalInputStream(MegaInputStream *inputStream);
    virtual m_off_t size();
    virtual bool read(byte *buffer, unsigned size);
};

class FileInputStream : public InputStreamAccess
{
    FileAccess *fileAccess;
    m_off_t offset;

public:
    FileInputStream(FileAccess *fileAccess);
    virtual m_off_t size();
    virtual bool read(byte *buffer, unsigned size);
    virtual ~FileInputStream();
};

#ifdef HAVE_LIBUV
class StreamingBuffer
{
public:
    StreamingBuffer();
    ~StreamingBuffer();
    void init(unsigned int capacity);
    unsigned int append(const char *buf, unsigned int len);
    unsigned int availableData();
    unsigned int availableSpace();
    unsigned int availableCapacity();
    uv_buf_t nextBuffer();
    void freeData(unsigned int len);
    void setMaxBufferSize(unsigned int bufferSize);
    void setMaxOutputSize(unsigned int outputSize);

    static const unsigned int MAX_BUFFER_SIZE = 2097152;
    static const unsigned int MAX_OUTPUT_SIZE = 16384;

protected:
    char *buffer;
    unsigned int capacity;
    unsigned int size;
    unsigned int free;
    unsigned int inpos;
    unsigned int outpos;
    unsigned int maxBufferSize;
    unsigned int maxOutputSize;
};

class MegaTCPServer;
class MegaTCPContext : public MegaTransferListener, public MegaRequestListener
{
public:
    MegaTCPContext();
    virtual ~MegaTCPContext();

    // Connection management
    MegaTCPServer *server;
    uv_tcp_t tcphandle;
    uv_async_t asynchandle;
    uv_mutex_t mutex;
    MegaApiImpl *megaApi;
    m_off_t bytesWritten;
    m_off_t size;
    char *lastBuffer;
    int lastBufferLen;
    bool nodereceived;
    bool finished;
    bool failed;
    bool pause;

#ifdef ENABLE_EVT_TLS
    //tls stuff:
    evt_tls_t *evt_tls;
    bool invalid;
#endif
    std::list<char*> writePointers;

    // Request information
    bool range;
    m_off_t rangeStart;
    m_off_t rangeEnd;
    m_off_t rangeWritten;
    MegaNode *node;
    std::string path;
    std::string nodehandle;
    std::string nodekey;
    std::string nodename;
    m_off_t nodesize;
    int resultCode;

};

class MegaTCPServer
{
protected:
    static void *threadEntryPoint(void *param);
    static http_parser_settings parsercfg;

    uv_loop_t uv_loop;

    set<handle> allowedHandles;
    handle lastHandle;
    list<MegaTCPContext*> connections;
    uv_async_t exit_handle;
    MegaApiImpl *megaApi;
    bool semaphoresdestroyed;
    uv_sem_t semaphoreStartup;
    uv_sem_t semaphoreEnd;
    MegaThread *thread;
    uv_tcp_t server;
    int maxBufferSize;
    int maxOutputSize;
    int restrictedMode;
    bool localOnly;
    bool started;
    int port;
    bool closing;
    int remainingcloseevents;

#ifdef ENABLE_EVT_TLS
    // TLS
    bool evtrequirescleaning;
    evt_ctx_t evtctx;
    std::string certificatepath;
    std::string keypath;
#endif

    // libuv callbacks
    static void onNewClient(uv_stream_t* server_handle, int status);
    static void onDataReceived(uv_stream_t* tcp, ssize_t nread, const uv_buf_t * buf);
    static void allocBuffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t* buf);
    static void onClose(uv_handle_t* handle);

#ifdef ENABLE_EVT_TLS
    //libuv tls
    static void onNewClient_tls(uv_stream_t* server_handle, int status);
    static void onWriteFinished_tls_async(uv_write_t* req, int status);
    static void on_tcp_read(uv_stream_t *stream, ssize_t nrd, const uv_buf_t *data);
    static int uv_tls_writer(evt_tls_t *evt_tls, void *bfr, int sz);
    static void on_evt_tls_close(evt_tls_t *evt_tls, int status);
    static void on_hd_complete( evt_tls_t *evt_tls, int status);
    static void evt_on_rd(evt_tls_t *evt_tls, char *bfr, int sz);
#endif


    static void onAsyncEventClose(uv_handle_t* handle);
    static void onAsyncEvent(uv_async_t* handle);
    static void onExitHandleClose(uv_handle_t* handle);

    static void onCloseRequested(uv_async_t* handle);

    static void onWriteFinished(uv_write_t* req, int status); //This might need to go to HTTPServer
#ifdef ENABLE_EVT_TLS
    static void onWriteFinished_tls(evt_tls_t *evt_tls, int status);
#endif
    static void closeConnection(MegaTCPContext *tcpctx);
    static void closeTCPConnection(MegaTCPContext *tcpctx);

    void run();
    void initializeAndStartListening();

    void answer(MegaTCPContext* tcpctx, const char *rsp, int rlen);


    //virtual methods:
    virtual void processReceivedData(MegaTCPContext *tcpctx, ssize_t nread, const uv_buf_t * buf);
    virtual void processAsyncEvent(MegaTCPContext *tcpctx);
    virtual MegaTCPContext * initializeContext(uv_stream_t *server_handle) = 0;
    virtual void processWriteFinished(MegaTCPContext* tcpctx, int status) = 0;
    virtual void processOnAsyncEventClose(MegaTCPContext* tcpctx);
    virtual bool respondNewConnection(MegaTCPContext* tcpctx) = 0; //returns true if server needs to start by reading
    virtual void processOnExitHandleClose(MegaTCPServer* tcpServer);

public:
    bool useTLS;
    MegaFileSystemAccess *fsAccess;

    std::string basePath;

    MegaTCPServer(MegaApiImpl *megaApi, std::string basePath, bool useTLS = false, std::string certificatepath = std::string(), std::string keypath = std::string());
    virtual ~MegaTCPServer();
    bool start(int port, bool localOnly = true);
    void stop(bool doNotWait = false);
    int getPort();
    bool isLocalOnly();
    void setMaxBufferSize(int bufferSize);
    void setMaxOutputSize(int outputSize);
    int getMaxBufferSize();
    int getMaxOutputSize();
    void setRestrictedMode(int mode);
    int getRestrictedMode();
    bool isHandleAllowed(handle h);
    void clearAllowedHandles();
    char* getLink(MegaNode *node, std::string protocol = "http");

    set<handle> getAllowedHandles();
    void removeAllowedHandle(MegaHandle handle);

    void readData(MegaTCPContext* tcpctx);
};


class MegaTCServer;
class MegaHTTPServer;
class MegaHTTPContext : public MegaTCPContext
{

public:
    MegaHTTPContext();
    ~MegaHTTPContext();

    // Connection management
    StreamingBuffer streamingBuffer;
    MegaTransferPrivate *transfer;
    http_parser parser;
    char *lastBuffer;
    int lastBufferLen;
    bool nodereceived;
    bool failed;
    bool pause;

    // Request information
    bool range;
    m_off_t rangeStart;
    m_off_t rangeEnd;
    m_off_t rangeWritten;
    MegaNode *node;
    std::string path;
    std::string nodehandle;
    std::string nodekey;
    std::string nodename;
    m_off_t nodesize;
    std::string nodepubauth;
    std::string nodeprivauth;
    int resultCode;


    // WEBDAV related
    int depth;
    std::string lastheader;
    std::string subpathrelative;
    const char *messageBody;
    size_t messageBodySize;
    std::string host;
    std::string destination;
    bool overwrite;
    FileAccess *tmpFileAccess;
    std::string tmpFileName;
    std::string newname; //newname for moved node
    MegaHandle nodeToMove; //node to be moved after delete
    MegaHandle newParentNode; //parent node for moved after delete

    uv_mutex_t mutex_responses;
    std::list<std::string> responses;

    virtual void onTransferStart(MegaApi *, MegaTransfer *transfer);
    virtual bool onTransferData(MegaApi *, MegaTransfer *transfer, char *buffer, size_t size);
    virtual void onTransferFinish(MegaApi* api, MegaTransfer *transfer, MegaError *e);
    virtual void onRequestFinish(MegaApi* api, MegaRequest *request, MegaError *e);
};

class MegaHTTPServer: public MegaTCPServer
{
protected:
    set<handle> allowedWebDavHandles;

    bool fileServerEnabled;
    bool folderServerEnabled;
    bool offlineAttribute;
    bool subtitlesSupportEnabled;

    //virtual methods:
    virtual void processReceivedData(MegaTCPContext *ftpctx, ssize_t nread, const uv_buf_t * buf);
    virtual void processAsyncEvent(MegaTCPContext *ftpctx);
    virtual MegaTCPContext * initializeContext(uv_stream_t *server_handle);
    virtual void processWriteFinished(MegaTCPContext* tcpctx, int status);
    virtual void processOnAsyncEventClose(MegaTCPContext* tcpctx);
    virtual bool respondNewConnection(MegaTCPContext* tcpctx);
    virtual void processOnExitHandleClose(MegaTCPServer* tcpServer);


    // HTTP parser callback
    static int onMessageBegin(http_parser* parser);
    static int onHeadersComplete(http_parser* parser);
    static int onUrlReceived(http_parser* parser, const char* url, size_t length);
    static int onHeaderField(http_parser* parser, const char* at, size_t length);
    static int onHeaderValue(http_parser* parser, const char* at, size_t length);
    static int onBody(http_parser* parser, const char* at, size_t length);
    static int onMessageComplete(http_parser* parser);

    static void sendHeaders(MegaHTTPContext *httpctx, string *headers);
    static void sendNextBytes(MegaHTTPContext *httpctx);
    static int streamNode(MegaHTTPContext *httpctx);

    //Utility funcitons
    static std::string getHTTPMethodName(int httpmethod);
    static std::string getHTTPErrorString(int errorcode);
    static std::string getResponseForNode(MegaNode *node, MegaHTTPContext* httpctx);

    // WEBDAV related
    static std::string getWebDavPropFindResponseForNode(std::string baseURL, std::string subnodepath, MegaNode *node, MegaHTTPContext* httpctx);
    static std::string getWebDavProfFindNodeContents(MegaNode *node, std::string baseURL, bool offlineAttribute);

    static void returnHttpCodeBasedOnRequestError(MegaHTTPContext* httpctx, MegaError *e, bool synchronous = true);
    static void returnHttpCode(MegaHTTPContext* httpctx, int errorCode, std::string errorMessage = string(), bool synchronous = true);

public:

    static void returnHttpCodeAsyncBasedOnRequestError(MegaHTTPContext* httpctx, MegaError *e);
    static void returnHttpCodeAsync(MegaHTTPContext* httpctx, int errorCode, std::string errorMessage = string());

    MegaHTTPServer(MegaApiImpl *megaApi, string basePath, bool useTLS = false, std::string certificatepath = std::string(), std::string keypath = std::string());
    virtual ~MegaHTTPServer();
    char *getWebDavLink(MegaNode *node);

    void clearAllowedHandles();
    bool isHandleWebDavAllowed(handle h);
    set<handle> getAllowedWebDavHandles();
    void removeAllowedWebDavHandle(MegaHandle handle);
    void enableFileServer(bool enable);
    void enableFolderServer(bool enable);
    bool isFileServerEnabled();
    bool isFolderServerEnabled();
    void enableOfflineAttribute(bool enable);
    bool isOfflineAttributeEnabled();
    bool isSubtitlesSupportEnabled();
    void enableSubtitlesSupport(bool enable);

};

class MegaFTPServer;
class MegaFTPDataServer;
class MegaFTPContext : public MegaTCPContext
{
public:

    int command;
    std::string arg1;
    std::string arg2;
    int resultcode;
    int pasiveport;
    MegaFTPDataServer * ftpDataServer;

    std::string tmpFileName;

    MegaNode *nodeToDeleteAfterMove;

    uv_mutex_t mutex_responses;
    std::list<std::string> responses;

    uv_mutex_t mutex_nodeToDownload;

    //status
    MegaHandle cwd;
    bool atroot;
    bool athandle;
    MegaHandle parentcwd;

    std::string cwdpath;

    MegaFTPContext();
    ~MegaFTPContext();

    virtual void onTransferStart(MegaApi *, MegaTransfer *transfer);
    virtual bool onTransferData(MegaApi *, MegaTransfer *transfer, char *buffer, size_t size);
    virtual void onTransferFinish(MegaApi* api, MegaTransfer *transfer, MegaError *e);
    virtual void onRequestFinish(MegaApi* api, MegaRequest *request, MegaError *e);
};

class MegaFTPDataServer;
class MegaFTPServer: public MegaTCPServer
{
protected:
    enum{
        FTP_CMD_INVALID = -1,
        FTP_CMD_USER = 1,
        FTP_CMD_PASS,
        FTP_CMD_ACCT,
        FTP_CMD_CWD,
        FTP_CMD_CDUP,
        FTP_CMD_SMNT,
        FTP_CMD_QUIT,
        FTP_CMD_REIN,
        FTP_CMD_PORT,
        FTP_CMD_PASV,
        FTP_CMD_TYPE,
        FTP_CMD_STRU,
        FTP_CMD_MODE,
        FTP_CMD_RETR,
        FTP_CMD_STOR,
        FTP_CMD_STOU,
        FTP_CMD_APPE,
        FTP_CMD_ALLO,
        FTP_CMD_REST,
        FTP_CMD_RNFR,
        FTP_CMD_RNTO,
        FTP_CMD_ABOR,
        FTP_CMD_DELE,
        FTP_CMD_RMD,
        FTP_CMD_MKD,
        FTP_CMD_PWD,
        FTP_CMD_LIST,
        FTP_CMD_NLST,
        FTP_CMD_SITE,
        FTP_CMD_SYST,
        FTP_CMD_STAT,
        FTP_CMD_HELP,
        FTP_CMD_FEAT,  //rfc2389
        FTP_CMD_SIZE,
        FTP_CMD_PROT,
        FTP_CMD_EPSV, //rfc2428
        FTP_CMD_PBSZ, //rfc2228
        FTP_CMD_OPTS, //rfc2389
        FTP_CMD_NOOP
    };

    std::string crlfout;

    MegaHandle nodeHandleToRename;

    int pport;
    int dataportBegin;
    int dataPortEnd;

    std::string getListingLineFromNode(MegaNode *child, std::string nameToShow = string());

    MegaNode *getBaseFolderNode(std::string path);
    MegaNode *getNodeByFullFtpPath(std::string path);
    void getPermissionsString(int permissions, char *permsString);


    //virtual methods:
    virtual void processReceivedData(MegaTCPContext *tcpctx, ssize_t nread, const uv_buf_t * buf);
    virtual void processAsyncEvent(MegaTCPContext *tcpctx);
    virtual MegaTCPContext * initializeContext(uv_stream_t *server_handle);
    virtual void processWriteFinished(MegaTCPContext* tcpctx, int status);
    virtual void processOnAsyncEventClose(MegaTCPContext* tcpctx);
    virtual bool respondNewConnection(MegaTCPContext* tcpctx);
    virtual void processOnExitHandleClose(MegaTCPServer* tcpServer);

public:

    std::string newNameAfterMove;

    MegaFTPServer(MegaApiImpl *megaApi, string basePath, int dataportBegin, int dataPortEnd, bool useTLS = false, std::string certificatepath = std::string(), std::string keypath = std::string());
    virtual ~MegaFTPServer();

    static std::string getFTPErrorString(int errorcode, std::string argument = string());

    static void returnFtpCodeBasedOnRequestError(MegaFTPContext* ftpctx, MegaError *e);
    static void returnFtpCode(MegaFTPContext* ftpctx, int errorCode, std::string errorMessage = string());

    static void returnFtpCodeAsyncBasedOnRequestError(MegaFTPContext* ftpctx, MegaError *e);
    static void returnFtpCodeAsync(MegaFTPContext* ftpctx, int errorCode, std::string errorMessage = string());
    MegaNode * getNodeByFtpPath(MegaFTPContext* ftpctx, std::string path);
    std::string cdup(handle parentHandle, MegaFTPContext* ftpctx);
    std::string cd(string newpath, MegaFTPContext* ftpctx);
    std::string shortenpath(std::string path);
};

class MegaFTPDataContext;
class MegaFTPDataServer: public MegaTCPServer
{
protected:

    //virtual methods:
    virtual void processReceivedData(MegaTCPContext *tcpctx, ssize_t nread, const uv_buf_t * buf);
    virtual void processAsyncEvent(MegaTCPContext *tcpctx);
    virtual MegaTCPContext * initializeContext(uv_stream_t *server_handle);
    virtual void processWriteFinished(MegaTCPContext* tcpctx, int status);
    virtual void processOnAsyncEventClose(MegaTCPContext* tcpctx);
    virtual bool respondNewConnection(MegaTCPContext* tcpctx);
    virtual void processOnExitHandleClose(MegaTCPServer* tcpServer);

    void sendNextBytes(MegaFTPDataContext *ftpdatactx);


public:
    MegaFTPContext *controlftpctx;

    std::string resultmsj;
    MegaNode *nodeToDownload;
    std::string remotePathToUpload;
    std::string newNameToUpload;
    MegaHandle newParentNodeHandle;
    m_off_t rangeStartREST;
    void sendData();
    bool notifyNewConnectionRequired;

    MegaFTPDataServer(MegaApiImpl *megaApi, string basePath, MegaFTPContext * controlftpctx, bool useTLS = false, std::string certificatepath = std::string(), std::string keypath = std::string());
    virtual ~MegaFTPDataServer();
    string getListingLineFromNode(MegaNode *child);
};

class MegaFTPDataServer;
class MegaFTPDataContext : public MegaTCPContext
{
public:

    MegaFTPDataContext();
    ~MegaFTPDataContext();

    void setControlCodeUponDataClose(int code, std::string msg = string());

    // Connection management
    StreamingBuffer streamingBuffer;
    MegaTransferPrivate *transfer;
    char *lastBuffer;
    int lastBufferLen;
    bool failed;
    int ecode;
    bool pause;
    MegaNode *node;

    m_off_t rangeStart;
    m_off_t rangeWritten;

    std::string tmpFileName;
    FileAccess* tmpFileAccess;
    size_t tmpFileSize;

    bool controlRespondedElsewhere;
    string controlResponseMessage;
    int controlResponseCode;

    virtual void onTransferStart(MegaApi *, MegaTransfer *transfer);
    virtual bool onTransferData(MegaApi *, MegaTransfer *transfer, char *buffer, size_t size);
    virtual void onTransferFinish(MegaApi* api, MegaTransfer *transfer, MegaError *e);
    virtual void onRequestFinish(MegaApi* api, MegaRequest *request, MegaError *e);
};



#endif

}

#endif //MEGAAPI_IMPL_H
