
#define DEBUG_HTTP 0x01
#define DEBUG_LUA  0x02
#define DEBUG_IO   0x03

extern int debugflags;

#define DEBUG(type, format, args...) {if (debugflags & (type)) printf(format, ## args); }

void pushButtonEvent(int button);
int getButtonState(int num);
void getState(char* buffer);

void setSignNr(int signNr, int nr);

int getSelectOptions(char* buffer);
void setSelectedOption(int i);
void getStatus(char* buffer);

void DoSleep(int mSec);
int getTicks();

void* ioLoop(void* data);
void startSignal(int signalNum, int length);
