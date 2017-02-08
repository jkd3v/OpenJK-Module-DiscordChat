static int _Text_Width(MultiPlugin_t *mpp, const char *text, float scale, int iMenuFont)
{
	return ((MultiSystem_t*)mpp->System)->R.Font.StrLenPixels(text, 1, scale);
}

static void _ChatBox_StrInsert(char *buffer, int place, char *str)
{
	int insLen = strlen(str);
	int i = strlen(buffer);
	int k = 0;

	buffer[i + insLen + 1] = 0; //terminate the string at its new length
	while (i >= place)
	{
		buffer[i + insLen] = buffer[i];
		i--;
	}

	i++;
	while (k < insLen)
	{
		buffer[i] = str[k];
		i++;
		k++;
	}
}

static void _ChatBox_AddString(MultiPlugin_t *mpp, char *chatStr)
{
	chatBoxItem_t *chat = &mpp->cg->chatItems[mpp->cg->chatItemActive];
	float chatLen;

	memset(chat, 0, sizeof(chatBoxItem_t));

	if (strlen(chatStr) > sizeof(chat->string))
	{ //too long, terminate at proper len.
		chatStr[sizeof(chat->string) - 1] = 0;
	}

	strcpy(chat->string, chatStr);
	chat->time = mpp->cg->time + 10000;

	chat->lines = 1;

	chatLen = _Text_Width(mpp, chat->string, 1.0f, FONT_SMALL);
	if (chatLen > 550)
	{ //we have to break it into segments...
		int i = 0;
		int lastLinePt = 0;
		char s[2];

		chatLen = 0;
		while (chat->string[i])
		{
			s[0] = chat->string[i];
			s[1] = 0;
			chatLen += _Text_Width(mpp, s, 0.65f, FONT_SMALL);

			if (chatLen >= 550)
			{
				int j = i;
				while (j > 0 && j > lastLinePt)
				{
					if (chat->string[j] == ' ')
					{
						break;
					}
					j--;
				}
				if (chat->string[j] == ' ')
				{
					i = j;
				}

				chat->lines++;
				_ChatBox_StrInsert(chat->string, i, "\n");
				i++;
				chatLen = 0;
				lastLinePt = i + 1;
			}
			i++;
		}
	}

	mpp->cg->chatItemActive++;
	if (mpp->cg->chatItemActive >= MAX_CHATBOX_ITEMS)
	{
		mpp->cg->chatItemActive = 0;
	}
}

static void addToCGameChat(MultiPlugin_t *mpp, char *str) {
	((MultiSystem_t*)mpp->System)->S.StartLocalSound(mpp->cgs->media.talkSound, CHAN_LOCAL_SOUND);
	_ChatBox_AddString(mpp, str);
	mpp->Com_Printf("*%s\n", str);
}