static void SCR_DrawChar(MultiPlugin_t *mpp, int x, int y, float size, int ch, qhandle_t *shader) {
	int row, col;
	float frow, fcol;
	float	ax, ay, aw, ah;

	ch &= 255;

	if (ch == ' ') {
		return;
	}

	if (y < -size) {
		return;
	}

	ax = x;
	ay = y;
	aw = size;
	ah = size;

	row = ch >> 4;
	col = ch & 15;

	float size2;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.03125;
	size2 = 0.0625;

	if (*shader == 0) *shader = ((MultiSystem_t*)mpp->System)->R.RegisterShaderNoMip("gfx/2d/charsgrid_med");
	((MultiSystem_t*)mpp->System)->R.DrawStretchPic(ax, ay, aw, ah,
		fcol, frow,
		fcol + size, frow + size2,
		*shader);
}

void SCR_DrawStringExt(MultiPlugin_t *mpp, int x, int y, float size, const char *string, qhandle_t *shader) {
	vec4_t		color;
	const char	*s;
	int			xx;

	// draw the drop shadow
	color[0] = color[1] = color[2] = 0;
	color[3] = 1;
	((MultiSystem_t*)mpp->System)->R.SetColor(color);
	s = string;
	xx = x;
	while (*s) {
		if (Q_IsColorString(s)) {
			s += 2;
			continue;
		}
		SCR_DrawChar(mpp, xx + 2, y + 2, size, *s, shader);
		xx += size;
		s++;
	}


	// draw the colored text
	s = string;
	xx = x;
	color[0] = color[1] = color[2] = color[3] = 1;
	((MultiSystem_t*)mpp->System)->R.SetColor(color);
	while (*s) {
		if (Q_IsColorString(s)) {
			if (Q_IsColorString(s)) {
				s += 2;
				continue;
			}
		}
		SCR_DrawChar(mpp, xx, y, size, *s, shader);
		xx += size;
		s++;
	}
	((MultiSystem_t*)mpp->System)->R.SetColor(NULL);
}