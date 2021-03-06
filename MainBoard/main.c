#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <dirent.h>

#ifdef WIN32
#include <windows.h>
#else
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#endif

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "main.h"
#include "http_server.h"

int debugflags;

FILE* logfile;

char curState[1024];
char curAdminState[1024];
int tickZero = 0;

void getState(char* buffer)
{
    if (tickZero == 0)
    {
        sprintf(buffer, "TIME:0|");
    }else{
        sprintf(buffer, "TIME:%i|", getTicks());
    }
	strcat(buffer, curState);
}

void getAdminState(char* buffer)
{
	strcpy(buffer, curAdminState);
}

int getTicks()
{
#ifdef WIN32
	return GetTickCount() - tickZero;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000 + tv.tv_usec / 1000) - tickZero;
#endif
}
void setNowIsTickZero()
{
	tickZero = 0;
	tickZero = getTicks();
}

void DoSleep(int mSec)
{
#ifdef WIN32
	Sleep(mSec);
#else
	usleep(mSec*1000);
#endif
}

#define MAX_BUTTON 128
struct TButtonEvent
{
	int button;
	int time;
};
struct TButtonEvent buttonEventList[MAX_BUTTON];
int buttonEventStart = 0;
int buttonEventEnd = 0;
pthread_mutex_t buttonEventMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t webActionMutex = PTHREAD_MUTEX_INITIALIZER;
char webActionBuffer[1024];

int luaSleep(lua_State* L)
{
	DoSleep(luaL_optnumber(L, 1, 100));
	return 0;
}

int luaGetTime(lua_State* L)
{
    if (lua_gettop(L) > 0)
    {
        int type = luaL_checknumber(L, 1);
        DEBUG(DEBUG_LUA, "setNowIsTickZero: %i\n", type);
        if (type)
        {
            setNowIsTickZero();
        }else{
            tickZero = 0;
        }
    }
	lua_pushnumber(L, getTicks());

	return 1;
}

int luaOpenLog(lua_State* L)
{
    char filename[PATH_MAX];
    sprintf(filename, "logs/%s.log", luaL_checkstring(L, 1));
	if (logfile)
    {
        fclose(logfile);
        logfile = NULL;
    }
    logfile = fopen(filename, "w");
    return 0;
}

int luaLog(lua_State* L)
{
	const char* str;
	int ticks = getTicks();

	str = luaL_checkstring(L, 1);
	if (logfile)
	{
        //printf("%03d.%03d: %s\n", ticks / 1000, ticks % 1000, str);
        fprintf(logfile, "%03d.%03d: %s\n", ticks / 1000, ticks % 1000, str);
	}else{
        //printf("??? %03d.%03d: %s\n", ticks / 1000, ticks % 1000, str);
	}
	return 0;
}

int luaCloseLog(lua_State* L)
{
    if (logfile)
    {
        fclose(logfile);
        logfile = NULL;
    }
    return 0;
}

int luaSetSignNr(lua_State* L)
{
    setSignNr(luaL_checkint(L, 1), luaL_checkint(L, 2));
    return 0;
}

int luaStartSignal(lua_State* L)
{
	int num = luaL_checknumber(L, 1);
	int length = luaL_checknumber(L, 2);
	DEBUG(DEBUG_LUA, "Start Signal: %i: %i\n", num, length);
	startSignal(num, length);
	return 0;
}

int luaClearButtonEvents(lua_State* L)
{
	(void)L;
	pthread_mutex_lock(&buttonEventMutex);
	buttonEventStart = 0;
	buttonEventEnd = 0;
	pthread_mutex_unlock(&buttonEventMutex);
	return 0;
}

int luaGetButtonEvent(lua_State* L)
{
	if (buttonEventStart == buttonEventEnd)
		return 0;

	pthread_mutex_lock(&buttonEventMutex);
	lua_pushnumber(L, buttonEventList[buttonEventStart].button);
	lua_pushnumber(L, buttonEventList[buttonEventStart].time);
	buttonEventStart = (buttonEventStart + 1) % MAX_BUTTON;
	pthread_mutex_unlock(&buttonEventMutex);
	return 2;
}

int luaGetWebAction(lua_State* L)
{
    pthread_mutex_lock(&webActionMutex);
    if (strlen(webActionBuffer) > 0)
    {
        lua_pushstring(L, webActionBuffer);
        webActionBuffer[0] = '\0';
        pthread_mutex_unlock(&webActionMutex);
        return 1;
    }
    pthread_mutex_unlock(&webActionMutex);
    return 0;
}

int luaGetButtonState(lua_State* L)
{
	int num = luaL_checknumber(L, 1);
	lua_pushnumber(L, getButtonState(num));
	return 1;
}

int luaSetState(lua_State* L)
{
	strcpy(curState, luaL_checkstring(L, 1));
	return 0;
}

int luaSetAdminState(lua_State* L)
{
	strcpy(curAdminState, luaL_checkstring(L, 1));
	return 0;
}

void pushButtonEvent(int button)
{
	int time = getTicks();

    pthread_mutex_lock(&buttonEventMutex);
	buttonEventList[buttonEventEnd].button = button;
	buttonEventList[buttonEventEnd].time = time;
	buttonEventEnd = (buttonEventEnd + 1) % MAX_BUTTON;
	pthread_mutex_unlock(&buttonEventMutex);
}

void* luaLoop(void* data)
{
	while(1)
	{
		lua_State* L = lua_open();

		if (luaL_dofile(L, "config.lua"))
		{
			fprintf(stderr, "%s", lua_tostring(L, -1));
			lua_pop(L, 1);  /* pop error message from the stack */
		}

		luaL_openlibs(L);
		lua_register(L, "sleep", luaSleep);
		lua_register(L, "openLog", luaOpenLog);
		lua_register(L, "log", luaLog);
		lua_register(L, "closeLog", luaCloseLog);
		lua_register(L, "getTime", luaGetTime);
		lua_register(L, "startSignal", luaStartSignal);
		lua_register(L, "clearButtonEvents", luaClearButtonEvents);
		lua_register(L, "getButtonEvent", luaGetButtonEvent);
		lua_register(L, "getButtonState", luaGetButtonState);
		lua_register(L, "setState", luaSetState);
		lua_register(L, "setAdminState", luaSetAdminState);
		lua_register(L, "setSignNr", luaSetSignNr);
		lua_register(L, "getWebAction", luaGetWebAction);

		if (luaL_dofile(L, "script.lua"))
		{
			fprintf(stderr, "%s", lua_tostring(L, -1));
			lua_pop(L, 1);  /* pop error message from the stack */
		}
        luaCloseLog(L);
		lua_close(L);
	}
	return NULL;
}

int httpALC(char* path, char* param, int socket)
{
	char buffer[1024];

	if (strcmp(path, "/button") == 0)
	{
		pushButtonEvent(atoi(param));
		httpSend(socket, buffer, sprintf(buffer, "OK:%i", atoi(param)));
		return 1;
	}
	if (strcmp(path, "/command") == 0)
	{
		httpSend(socket, buffer, sprintf(buffer, "OK"));
		return 1;
	}
	if (strcmp(path, "/state") == 0)
	{
		getState(buffer);
		httpSend(socket, buffer, strlen(buffer));
		return 1;
	}
	if (strcmp(path, "/adminstate") == 0)
	{
		getAdminState(buffer);
		httpSend(socket, buffer, strlen(buffer));
		return 1;
	}
	if (strcmp(path, "/action") == 0)
	{
        pthread_mutex_lock(&webActionMutex);
		strncpy(webActionBuffer, param, 1024);
		webActionBuffer[1024-1] = '\0';
		pthread_mutex_unlock(&webActionMutex);
		return 1;
	}
	if (strcmp(path, "/loglist") == 0)
	{
        DIR* dh = opendir("logs");
        struct dirent *entry;
        while((entry=readdir(dh))!=NULL)
        {
            if (entry->d_name[0] == '.')
                continue;
            httpSend(socket, entry->d_name, strlen(entry->d_name));
            httpSend(socket, "\n", 1);
        }
        closedir(dh);
		return 1;
	}
	if (strcmp(path, "/log") == 0)
	{
        FILE* f;
        char filename[PATH_MAX];
        snprintf(filename, PATH_MAX, "logs/%s", param);
        filename[PATH_MAX - 1] = '\0';
        f = fopen(filename, "rb");
        if (f)
        {
            char buffer[4096];
            int size;
            while((size = fread(buffer, 1, sizeof(buffer), f)) > 0)
            {
                httpSend(socket, buffer, size);
            }
            fclose(f);
            return 1;
        }
	}
	return 0;
}

int main (int argc, char *argv[])
{
	pthread_t luaThread;
    int i;
    for(i=1;i<argc;i++)
    {
        if (strcmp(argv[i], "-h") == 0)
        {
            printf("Options\n");
            printf("-dhttp: debug HTTP\n");
            printf("-dlua: debug LUA\n");
            printf("-dio: debug IO\n");
            return 0;
        }else if (strcmp(argv[i], "-dhttp") == 0)
        {
            debugflags |= DEBUG_HTTP;
        }else if (strcmp(argv[i], "-dlua") == 0)
        {
            debugflags |= DEBUG_LUA;
        }else if (strcmp(argv[i], "-dio") == 0)
        {
            debugflags |= DEBUG_IO;
        }
    }

	registerHttpHandler(httpALC);

	pthread_create(&luaThread, NULL, luaLoop, NULL);
	pthread_create(&luaThread, NULL, serverLoop, NULL);
	pthread_create(&luaThread, NULL, ioLoop, NULL);

	pthread_exit(0);
	return 0;
}
