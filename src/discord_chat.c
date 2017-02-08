#include "cgame/cg_local.h"
#include "ui/keycodes.h"
#include "mppShared.h"
#include "portable_mutex.h"
#include "portable_socket.h"
#include "portable_thread.h"
#include "mpp_cg_chat.h"
#include "mpp_scr_draw.h"

#define sleep(sec) Sleep(sec*1000)

#define CHAT_PREFIX "afj"
#define CHAT_NAME "AFJ"
#define CHAT_NAME_LENGTH 3
#define CHAT_HOSTNAME "92.222.92.243"
#define CHAT_PORT "29069"

#define CHAT_TYPE_CONNECT	'0'
#define CHAT_TYPE_RENAME	'1'
#define CHAT_TYPE_MESSAGE	'2'
#define CHAT_TYPE_WHOIS		'3'

#define CHAT_MAX_CHARS_PER_LINE 30

#define MESSAGE_PREFIX S_COLOR_GREY "[" CHAT_NAME "] " S_COLOR_WHITE
#define MESSAGE_SUFFIX S_COLOR_WHITE ": "
#define MESSAGE_MAX_LENGTH (MAX_SAY_TEXT - MAX_NAME_LENGTH - CHAT_NAME_LENGTH - 11)

#define STATE_DESTROYING			(1 << 0)
#define STATE_RUNNING				(1 << 1)
#define STATE_DIDFIRSTCONNECTION	(1 << 2)
#define STATE_USINGCHAT				(1 << 3)
#define STATE_DRAWSELECTOR			(1 << 4)

#define SELECTOR_TICK_RATE 500

// TODO: externaliser les func utiles

static void chat_connect();
static void chat_dropped();
static void chat_send(char type, char *message);
static portable_thread_f_return chat_thread(portable_thread_f_parameter args);
static void chat_say();
static void chat_whois();
static void chat_start();
static void chat_disconnect();
static void chat_reconnect();

static MultiPlugin_t	*sys;
static MultiSystem_t	*trap;

static vmCvar_t cvar_autoconnect;
static vmCvar_t cvar_hostname;
static vmCvar_t cvar_port;

static portable_mutex _mutex;
static SOCKET _sock = INVALID_SOCKET;
static portable_thread_id _threadId = 0;
static portable_thread _thread = NULL;
static char state = 0;
static int _selectorNextTick = 0;
static char _savedName[MAX_NAME_LENGTH];
static char _chatHandle[MESSAGE_MAX_LENGTH];
static unsigned short _selector;
static qhandle_t _chatShader = 0;
static char _messagesToAdd[20][MESSAGE_MAX_LENGTH];
static unsigned short _nbMessagesToAdd;



static void chat_connect() {
	_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (_sock != INVALID_SOCKET)
	{
		struct hostent *hostinfo = gethostbyname(cvar_hostname.string);
		if (hostinfo != NULL)
		{
			SOCKADDR_IN sin = { 0 };
			sin.sin_addr = *(IN_ADDR *)hostinfo->h_addr;
			sin.sin_port = htons(cvar_port.integer);
			sin.sin_family = AF_INET;

			if (connect(_sock, (SOCKADDR *)&sin, sizeof(SOCKADDR)) != SOCKET_ERROR)
			{
				sys->Q_strncpyz(_savedName, sys->clientInfo[sys->cg->clientNum].name, MAX_NAME_LENGTH);
				chat_send(CHAT_TYPE_CONNECT, sys->clientInfo[sys->cg->clientNum].name);
				sys->Com_Printf(S_COLOR_GREY CHAT_NAME " - " S_COLOR_WHITE "Chat connexion [" S_COLOR_GREEN "SUCCESS" S_COLOR_WHITE "]\n");
			}
			else {
				chat_dropped();
			}
		}
	}
}

static void chat_dropped() {
	_sock = INVALID_SOCKET;
	sys->Com_Printf(S_COLOR_GREY CHAT_NAME " - " S_COLOR_WHITE "Chat connexion [" S_COLOR_RED "LOST" S_COLOR_WHITE "]\n");
}

static void chat_send(char type, char *message) {
	char buffer[MESSAGE_MAX_LENGTH + 1];
	sys->Com_Sprintf(buffer, sizeof buffer, "%c%s", type, message);
	if (send(_sock, buffer, strlen(buffer), 0) < 0) {
		chat_dropped();
	}
}

static portable_thread_f_return chat_thread(portable_thread_f_parameter args) {
	char buffer[MAX_SAY_TEXT];
	int n = 0;

	while (!(state & STATE_DESTROYING)) {
		while (_sock == INVALID_SOCKET && !(state & STATE_DESTROYING)) {
			chat_connect();
			sleep(5);
		}

		while (_sock != INVALID_SOCKET && !(state & STATE_DESTROYING)) {
			if ((n = recv(_sock, buffer, sizeof buffer - 1, 0)) > 0)
			{
				// New message
				buffer[n] = '\0';
				portable_mutex_lock(_mutex);
				while (_nbMessagesToAdd >= 20) {
					portable_mutex_unlock(_mutex);
					sleep(1);
					portable_mutex_lock(_mutex);
				}
				sys->Q_strncpyz(_messagesToAdd[_nbMessagesToAdd], buffer, MESSAGE_MAX_LENGTH);
				_nbMessagesToAdd++;
				portable_mutex_unlock(_mutex);
			}
			else if (n < 0) chat_dropped();
		}
	}

	return 0;
}

static void chat_say() {
	if (_sock == INVALID_SOCKET) return;

	char message[MESSAGE_MAX_LENGTH] = "\0";
	for (int i = 1; i <= sys->Cmd_Argc(); i++)
		sys->Com_Sprintf(message, MESSAGE_MAX_LENGTH, (i == 1 ? "%s%s" : "%s %s"), message, sys->Cmd_Argv(i));
	chat_send(CHAT_TYPE_MESSAGE, message);

	if (_sock != INVALID_SOCKET) {
		addToCGameChat(sys, sys->va(MESSAGE_PREFIX "%s" MESSAGE_SUFFIX "%s", sys->clientInfo[sys->cg->clientNum].name, message));
	}
	else {
		sys->Com_Printf(S_COLOR_GREY CHAT_NAME " - " S_COLOR_WHITE "Impossible to send your message, you are not connected to the server.\n");
	}
}

static void chat_whois() {
	chat_send(CHAT_TYPE_WHOIS, "");
}

static void chat_start() {
	state |= STATE_DIDFIRSTCONNECTION;
	if (!(state & STATE_RUNNING)) {
		socket_init();
		_nbMessagesToAdd = 0;
		portable_mutex_create(_mutex);
		portable_thread_create(_thread, _threadId, chat_thread, NULL);
		state |= STATE_RUNNING;
	}
}

static void chat_disconnect() {
	state |= STATE_DESTROYING;
	if (_sock != INVALID_SOCKET) closesocket(_sock);
	if (_thread) {
		portable_thread_wait(_thread);
		portable_thread_clear(_thread);
	}
	socket_destroy();
	portable_mutex_destroy(_mutex);
	state &= ~STATE_DESTROYING;
	state &= ~STATE_RUNNING;
}

static void chat_reconnect() {
	chat_disconnect();
	chat_start();
}

static void chat_use() {
	_selector = 0;
	state |= STATE_USINGCHAT;
	state |= STATE_DRAWSELECTOR;
	_selectorNextTick = sys->cg->time - sys->cgs->levelStartTime + SELECTOR_TICK_RATE;
	memset(_chatHandle, '\0', sizeof _chatHandle);
	trap->Key.SetCatcher(trap->Key.GetCatcher() | KEYCATCH_CGAME);
}



/**************************************************
* mpp
*
* Plugin exported function. This function gets called
* the moment the module is loaded and provides a
* pointer to a shared structure. Store the pointer
* and copy the System pointer safely.
**************************************************/

__declspec(dllexport) void mpp(MultiPlugin_t *pPlugin)
{
	sys = pPlugin;
	trap = sys->System;

	trap->Cvar.Register(&cvar_autoconnect, CHAT_PREFIX "_chat_autoconnect", "1", CVAR_ARCHIVE);
	trap->Cvar.Register(&cvar_hostname, CHAT_PREFIX "_chat_hostname", CHAT_HOSTNAME, CVAR_ARCHIVE);
	trap->Cvar.Register(&cvar_port, CHAT_PREFIX "_chat_port", CHAT_PORT, CVAR_ARCHIVE);

	trap->AddCommand(CHAT_PREFIX "_chat", chat_use, "The " CHAT_NAME " chat");
	trap->AddCommand(CHAT_PREFIX "_say", chat_say, "Say something to the " CHAT_NAME " chat");
	trap->AddCommand(CHAT_PREFIX "_whois", chat_whois, "Show who is online on the " CHAT_NAME " chat");
	trap->AddCommand(CHAT_PREFIX "_connect", chat_start, "Connect to the " CHAT_NAME " chat");
	trap->AddCommand(CHAT_PREFIX "_reconnect", chat_reconnect, "Reconnect to the " CHAT_NAME " chat");
	trap->AddCommand(CHAT_PREFIX "_disconnect", chat_disconnect, "Disconnect from the " CHAT_NAME " chat");

	if (cvar_autoconnect.integer == 0) state |= STATE_DIDFIRSTCONNECTION;
}




__declspec(dllexport) int mppPreMain(int cmd, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11)
{
	switch (cmd)
	{
		case CG_SHUTDOWN:
		{
			chat_disconnect();
			if (cvar_autoconnect.integer == 0) state |= STATE_DIDFIRSTCONNECTION;
			break;
		}
		case CG_KEY_EVENT:
		{
			if ((state & STATE_USINGCHAT) && arg1) {
				// ADDED STRING TO CHAT
				int key = arg0 & ~K_CHAR_FLAG;
				int len = strlen(_chatHandle);
				if (arg0 == A_ESCAPE) {
					state &= ~STATE_USINGCHAT;
					trap->Key.SetCatcher(trap->Key.GetCatcher() & ~KEYCATCH_CGAME);
				}
				else if (arg0 == A_ENTER || arg0 == A_KP_ENTER) {
					chat_send(CHAT_TYPE_MESSAGE, _chatHandle);
					state &= ~STATE_USINGCHAT;
					trap->Key.SetCatcher(trap->Key.GetCatcher() & ~KEYCATCH_CGAME);
				}
				else if (arg0 == A_DELETE) { // SUPR
					if (_selector < len) sys->Com_Sprintf(_chatHandle, MESSAGE_MAX_LENGTH, "%.*s%s", _selector, sys->va("%s", _chatHandle), sys->va("%s", _chatHandle) + _selector + 1);
				}
				else if (arg0 == A_CURSOR_LEFT) { // LEFT
					if (_selector > 0) _selector--;
				}
				else if (arg0 == A_CURSOR_RIGHT) { // RIGHT
					if (_selector < len) _selector++;
				}
				else if (arg0 == A_CURSOR_UP) { // UP
					if (_selector - CHAT_MAX_CHARS_PER_LINE < 0) _selector = 0;
					else _selector -= CHAT_MAX_CHARS_PER_LINE;
				}
				else if (arg0 == A_CURSOR_DOWN) { // DOWN
					if (_selector + CHAT_MAX_CHARS_PER_LINE > len) _selector = len;
					else _selector += CHAT_MAX_CHARS_PER_LINE;
				}
				else if (arg0 == A_HOME) { // ORIGIN/HOME
					_selector = 0;
				}
				else if (arg0 == A_END) { // END
					_selector = len;
				}
				else if (arg0 == A_BACKSPACE) { // DEL
					if (_selector > 0) {
						sys->Com_Sprintf(_chatHandle, MESSAGE_MAX_LENGTH, "%.*s%s", _selector - 1, sys->va("%s", _chatHandle), sys->va("%s", _chatHandle) + _selector);
						_selector--;
					}
				}
				else if (key >= 32 && key != arg0 && len < MESSAGE_MAX_LENGTH-1) {
					if (_selector == len) sys->Com_Sprintf(_chatHandle, MESSAGE_MAX_LENGTH, "%s%c", sys->va("%s", _chatHandle), key);
					else sys->Com_Sprintf(_chatHandle, MESSAGE_MAX_LENGTH, "%.*s%c%s", _selector, sys->va("%s", _chatHandle), key, sys->va("%s", _chatHandle) + _selector);
					_selector++;
				}
				return qtrue; // Break normal process and stop there
			}
			break;
		}
		case CG_EVENT_HANDLING:
		{
			if ((state & STATE_USINGCHAT) && arg0 == CGAME_EVENT_NONE) {
				// User pressed ESCAPE
				state &= ~STATE_USINGCHAT;
				trap->Key.SetCatcher(trap->Key.GetCatcher() & ~KEYCATCH_CGAME);
				return qtrue;
			}
			break;
		}
		default:
		{
			if (!(state & STATE_DIDFIRSTCONNECTION) && cmd != CG_SHUTDOWN) {
				chat_start();
			}
			break;
		}
	}
	

	return sys->noBreakCode;
}

__declspec(dllexport) int mppPostMain(int cmd, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11) {
	if (cmd == CG_DRAW_ACTIVE_FRAME) {
		if ((state & STATE_USINGCHAT)) {
			int len = strlen(_chatHandle);
			// Draw text
			SCR_DrawStringExt(sys, 10, 0, BIGCHAR_WIDTH, sys->va(CHAT_NAME ": %.*s", CHAT_MAX_CHARS_PER_LINE - CHAT_NAME_LENGTH - 2, _chatHandle), &_chatShader);
			for (int i = 1; i*CHAT_MAX_CHARS_PER_LINE < len + CHAT_NAME_LENGTH + 2; i++) {
				SCR_DrawStringExt(sys, 10, i*BIGCHAR_WIDTH, BIGCHAR_WIDTH, sys->va("%.*s", CHAT_MAX_CHARS_PER_LINE, _chatHandle + CHAT_MAX_CHARS_PER_LINE - CHAT_NAME_LENGTH - 2 + (i - 1)*CHAT_MAX_CHARS_PER_LINE), &_chatShader);
			}

			// Draw selector
			if (_selectorNextTick < sys->cg->time - sys->cgs->levelStartTime) {
				state ^= STATE_DRAWSELECTOR;
				_selectorNextTick = sys->cg->time - sys->cgs->levelStartTime + SELECTOR_TICK_RATE;
			}
			if (state & STATE_DRAWSELECTOR) {
				int selectorPos = CHAT_NAME_LENGTH + 2 + _selector;
				SCR_DrawStringExt(sys, 10 + BIGCHAR_WIDTH*(selectorPos%CHAT_MAX_CHARS_PER_LINE), 2 + (selectorPos / CHAT_MAX_CHARS_PER_LINE)*BIGCHAR_WIDTH, BIGCHAR_WIDTH, "_", &_chatShader);
			}
		}

		// Display new messages
		portable_mutex_lock(_mutex);
		for (; _nbMessagesToAdd > 0; _nbMessagesToAdd--) {
			addToCGameChat(sys, _messagesToAdd[_nbMessagesToAdd - 1]);
		}
		portable_mutex_unlock(_mutex);
	}

	return sys->noBreakCode;
}

__declspec(dllexport) int mppPostSystem(int *args) {
	switch (args[0])
	{
	case CG_CVAR_SET:
	case CG_CVAR_UPDATE:
	{
		trap->Cvar.Update(&cvar_autoconnect);
		trap->Cvar.Update(&cvar_hostname);
		trap->Cvar.Update(&cvar_port);
		break;
	}
	case CG_GETGAMESTATE:
	{
		if (sys->Q_stricmp(_savedName, sys->clientInfo[sys->cg->clientNum].name) != 0) {
			sys->Q_strncpyz(_savedName, sys->clientInfo[sys->cg->clientNum].name, MAX_NAME_LENGTH);
			chat_send(CHAT_TYPE_RENAME, _savedName);
		}
		break;
	}
	default:
		break;
	}

	return sys->noBreakCode;
}