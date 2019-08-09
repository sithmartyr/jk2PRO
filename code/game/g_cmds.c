// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"

#include "../ui/menudef.h"			// for the voice chats

//rww - for getting bot commands...
int AcceptBotCommand(char *cmd, gentity_t *pl);
//end rww

extern void AddIP(char *str);

void BG_CycleInven(playerState_t *ps, int direction);
void BG_CycleForce(playerState_t *ps, int direction);

/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage( gentity_t *ent ) {
	char		entry[1024];
	char		string[1400];
	int			stringlength;
	int			i, j;
	gclient_t	*cl;
	int			numSorted, scoreFlags, accuracy, perfect;

	// send the latest information on all clients
	string[0] = 0;
	stringlength = 0;
	scoreFlags = 0;

	numSorted = level.numConnectedClients;
	
	if (numSorted > MAX_CLIENT_SCORE_SEND)
	{
		numSorted = MAX_CLIENT_SCORE_SEND;
	}

	for (i=0 ; i < numSorted ; i++) {
		int		ping;

		cl = &level.clients[level.sortedClients[i]];

		if ( cl->pers.connected == CON_CONNECTING ) {
			ping = -1;
		} else {
			ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
		}

		if( cl->accuracy_shots ) {
			accuracy = cl->accuracy_hits * 100 / cl->accuracy_shots;
		}
		else {
			accuracy = 0;
		}
		perfect = ( cl->ps.persistant[PERS_RANK] == 0 && cl->ps.persistant[PERS_KILLED] == 0 ) ? 1 : 0;

		Com_sprintf (entry, sizeof(entry),
			" %i %i %i %i %i %i %i %i %i %i %i %i %i %i", level.sortedClients[i],
			cl->ps.persistant[PERS_SCORE], ping, (level.time - cl->pers.enterTime)/60000,
			scoreFlags, g_entities[level.sortedClients[i]].s.powerups, accuracy, 
			cl->ps.persistant[PERS_IMPRESSIVE_COUNT],
			cl->ps.persistant[PERS_EXCELLENT_COUNT],
			cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT], 
			cl->ps.persistant[PERS_DEFEND_COUNT], 
			cl->ps.persistant[PERS_ASSIST_COUNT], 
			perfect,
			cl->ps.persistant[PERS_CAPTURES]);
		j = strlen(entry);
		if (stringlength + j > 1022)
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}

	//still want to know the total # of clients
	i = level.numConnectedClients;

	trap_SendServerCommand( ent-g_entities, va("scores %i %i %i%s", i, 
		level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE],
		string ) );
}


/*
==================
Cmd_Score_f

Request current scoreboard information
==================
*/
void Cmd_Score_f( gentity_t *ent ) {
	DeathmatchScoreboardMessage( ent );
}



/*
==================
CheatsOk
==================
*/
qboolean	CheatsOk( gentity_t *ent ) {
	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOCHEATS")));
		return qfalse;
	}
	if ( ent->health <= 0 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "MUSTBEALIVE")));
		return qfalse;
	}
	return qtrue;
}


/*
==================
ConcatArgs
==================
*/
char	*ConcatArgs( int start ) {
	int		i, c, tlen;
	static char	line[MAX_STRING_CHARS];
	int		len;
	char	arg[MAX_STRING_CHARS];

	len = 0;
	c = trap_Argc();
	for ( i = start ; i < c ; i++ ) {
		trap_Argv( i, arg, sizeof( arg ) );
		tlen = strlen( arg );
		if ( len + tlen >= MAX_STRING_CHARS - 1 ) {
			break;
		}
		memcpy( line + len, arg, tlen );
		len += tlen;
		if ( i != c - 1 ) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

/*
==================
SanitizeString

Remove case and control characters
==================
*/
void SanitizeString( char *in, char *out ) {
	while ( *in ) {
		if ( *in == 27 ) {
			in += 2;		// skip color code
			continue;
		}
		if ( *in < 32 ) {
			in++;
			continue;
		}
		*out++ = tolower( *in++ );
	}

	*out = 0;
}

//[JAPRO - Serverside - All - Redid and moved sanitizestring2 for partial name recognition - Start]
/*
==================
SanitizeString2

Rich's revised version of SanitizeString
==================
*/
void SanitizeString2(const char *in, char *out)
{
	int i = 0;
	int r = 0;

	while (in[i])
	{
		if (i >= MAX_NAME_LENGTH - 1)
		{ //the ui truncates the name here..
			break;
		}

		if (in[i] == '^')
		{
			if (in[i + 1] >= 48 && //'0'
				in[i + 1] <= 57) //'9'
			{ //only skip it if there's a number after it for the color
				i += 2;
				continue;
			}
			else
			{ //just skip the ^
				i++;
				continue;
			}
		}

		if (in[i] < 32)
		{
			i++;
			continue;
		}

		out[r] = tolower(in[i]);//lowercase please
		r++;
		i++;
	}
	out[r] = 0;
}
//[JAPRO - Serverside - All - Redid and moved sanitizestring2 for partial name recognition - End]

int JP_ClientNumberFromString(gentity_t *to, const char *s)
{
	gclient_t	*cl;
	int			idnum, i, match = -1;
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];
	idnum = atoi(s);


	//redo
	/*
	if (!Q_stricmp(s, "0")) {
	cl = &level.clients[idnum];
	if ( cl->pers.connected != CON_CONNECTED ) {
	trap_SendServerCommand( to-g_entities, va("print \"Client '%i' is not active\n\"", idnum));
	return -1;
	}
	return 0;
	}
	if (idnum && idnum < 32) {
	cl = &level.clients[idnum];
	if ( cl->pers.connected != CON_CONNECTED ) {
	trap_SendServerCommand( to-g_entities, va("print \"Client '%i' is not active\n\"", idnum));
	return -1;
	}
	return idnum;
	}
	*/
	//end redo

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9' && strlen(s) == 1) //changed this to only recognize numbers 0-31 as client numbers, otherwise interpret as a name, in which case sanitize2 it and accept partial matches (return error if multiple matches)
	{
		idnum = atoi(s);
		cl = &level.clients[idnum];
		if (cl->pers.connected != CON_CONNECTED) {
			trap_SendServerCommand(to - g_entities, va("print \"Client '%i' is not active\n\"", idnum));
			return -1;
		}
		return idnum;
	}

	if ((s[0] == '1' || s[0] == '2') && (s[1] >= '0' && s[1] <= '9' && strlen(s) == 2))  //changed and to or ..
	{
		idnum = atoi(s);
		cl = &level.clients[idnum];
		if (cl->pers.connected != CON_CONNECTED) {
			trap_SendServerCommand(to - g_entities, va("print \"Client '%i' is not active\n\"", idnum));
			return -1;
		}
		return idnum;
	}

	if (s[0] == '3' && (s[1] >= '0' && s[1] <= '1' && strlen(s) == 2))
	{
		idnum = atoi(s);
		cl = &level.clients[idnum];
		if (cl->pers.connected != CON_CONNECTED) {
			trap_SendServerCommand(to - g_entities, va("print \"Client '%i' is not active\n\"", idnum));
			return -1;
		}
		return idnum;
	}




	// check for a name match
	SanitizeString2(s, s2);
	for (idnum = 0, cl = level.clients; idnum < level.maxclients; idnum++, cl++) {
		if (cl->pers.connected != CON_CONNECTED) {
			continue;
		}
		SanitizeString2(cl->pers.netname, n2);

		for (i = 0; i < level.numConnectedClients; i++)
		{
			cl = &level.clients[level.sortedClients[i]];
			SanitizeString2(cl->pers.netname, n2);
			if (strstr(n2, s2))
			{
				if (match != -1)
				{ //found more than one match
					trap_SendServerCommand(to - g_entities, va("print \"More than one user '%s' on the server\n\"", s));
					return -2;
				}
				match = level.sortedClients[i];
			}
		}
		if (match != -1)//uhh
			return match;
	}
	if (!atoi(s)) //Uhh.. well.. whatever. fixes amtele spam problem when teleporting to x y z yaw
		trap_SendServerCommand(to - g_entities, va("print \"User '%s' is not on the server\n\"", s));
	return -1;
}
//[JAPRO - Serverside - All - Redid and clientnumberfromstring for partial name recognition - End]

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
int ClientNumberFromString( gentity_t *to, char *s ) {
	gclient_t	*cl;
	int			idnum;
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9') {
		idnum = atoi( s );
		if ( idnum < 0 || idnum >= level.maxclients ) {
			trap_SendServerCommand( to-g_entities, va("print \"Bad client slot: %i\n\"", idnum));
			return -1;
		}

		cl = &level.clients[idnum];
		if ( cl->pers.connected != CON_CONNECTED ) {
			trap_SendServerCommand( to-g_entities, va("print \"Client %i is not active\n\"", idnum));
			return -1;
		}
		return idnum;
	}

	// check for a name match
	SanitizeString( s, s2 );
	for ( idnum=0,cl=level.clients ; idnum < level.maxclients ; idnum++,cl++ ) {
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		SanitizeString( cl->pers.netname, n2 );
		if ( !strcmp( n2, s2 ) ) {
			return idnum;
		}
	}

	trap_SendServerCommand( to-g_entities, va("print \"User %s is not on the server\n\"", s));
	return -1;
}

qboolean G_AdminUsableOn(gclient_t *ourClient, gclient_t *theirClient, unsigned int adminCmd) {
	if (!ourClient || !theirClient)
		return qfalse;
	if ((theirClient->sess.accountFlags & adminCmd) && ourClient != theirClient) //He has that cmd so... 
		return qfalse;
	return qtrue;
}

int G_AdminAllowed(gentity_t *ent, unsigned int adminCmd, qboolean cheatAllowed, qboolean raceAllowed, char *cmdName) {
	//See if they are allowed to use a cmd.  first check their adminLevel, then check their account admin
	//3 = allowed by admin, 2 = allowed by racemode, 1 = allowed by cheats

	if (ent->client->sess.accountFlags & adminCmd)
		return 3;
	if (raceAllowed && ent->client->sess.raceMode)
		return 2;
	if (cheatAllowed && sv_cheats.integer)
		return 1;

	if (cmdName) {
		if (raceAllowed && jp_raceMode.integer)
			trap_SendServerCommand(ent - g_entities, va("print \"You are not authorized to use this command (%s) outside of racemode.\n\"", cmdName));
		else if (cheatAllowed)
			trap_SendServerCommand(ent - g_entities, "print \"Cheats are not enabled on this server.\n\"");
		else
			trap_SendServerCommand(ent - g_entities, va("print \"You are not authorized to use this command (%s).\n\"", cmdName));
	}

	return 0;
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f (gentity_t *ent)
{
	char		name[MAX_TOKEN_CHARS];
	gitem_t		*it;
	int			i;
	qboolean	give_all;
	gentity_t		*it_ent;
	trace_t		trace;
	char		arg[MAX_TOKEN_CHARS];

	if ( !CheatsOk( ent ) ) {
		return;
	}

	trap_Argv( 1, name, sizeof( name ) );

	if (Q_stricmp(name, "all") == 0)
		give_all = qtrue;
	else
		give_all = qfalse;

	if (give_all)
	{
		i = 0;
		while (i < HI_NUM_HOLDABLE)
		{
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << i);
			i++;
		}
		i = 0;
	}

	if (give_all || Q_stricmp( name, "health") == 0)
	{
		if (trap_Argc() == 3) {
			trap_Argv( 2, arg, sizeof( arg ) );
			ent->health = atoi(arg);
			if (ent->health > ent->client->ps.stats[STAT_MAX_HEALTH]) {
				ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
			}
		}
		else {
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		ent->client->ps.stats[STAT_WEAPONS] = (1 << (WP_DET_PACK+1))  - ( 1 << WP_NONE );
		if (!give_all)
			return;
	}
	
	if ( !give_all && Q_stricmp(name, "weaponnum") == 0 )
	{
		trap_Argv( 2, arg, sizeof( arg ) );
		ent->client->ps.stats[STAT_WEAPONS] |= (1 << atoi(arg));
		return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		int num = 999;
		if (trap_Argc() == 3) {
			trap_Argv( 2, arg, sizeof( arg ) );
			num = atoi(arg);
		}
		for ( i = 0 ; i < MAX_WEAPONS ; i++ ) {
			ent->client->ps.ammo[i] = num;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		if (trap_Argc() == 3) {
			trap_Argv( 2, arg, sizeof( arg ) );
			ent->client->ps.stats[STAT_ARMOR] = atoi(arg);
		} else {
			ent->client->ps.stats[STAT_ARMOR] = ent->client->ps.stats[STAT_MAX_HEALTH];
		}

		if (!give_all)
			return;
	}

	if (Q_stricmp(name, "excellent") == 0) {
		ent->client->ps.persistant[PERS_EXCELLENT_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "impressive") == 0) {
		ent->client->ps.persistant[PERS_IMPRESSIVE_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "gauntletaward") == 0) {
		ent->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "defend") == 0) {
		ent->client->ps.persistant[PERS_DEFEND_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "assist") == 0) {
		ent->client->ps.persistant[PERS_ASSIST_COUNT]++;
		return;
	}

	// spawn a specific item right on the player
	if ( !give_all ) {
		it = BG_FindItem (name);
		if (!it) {
			return;
		}

		it_ent = G_Spawn(qtrue);
		VectorCopy( ent->r.currentOrigin, it_ent->s.origin );
		it_ent->classname = it->classname;
		G_SpawnItem (it_ent, it);
		FinishSpawningItem(it_ent );
		memset( &trace, 0, sizeof( trace ) );
		Touch_Item (it_ent, ent, &trace);
		if (it_ent->inuse) {
			G_FreeEntity( it_ent );
		}
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f (gentity_t *ent)
{
	char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f( gentity_t *ent ) {
	char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}


void DeletePlayerProjectiles(gentity_t *ent) {
	int i;
	for (i = MAX_CLIENTS; i<MAX_GENTITIES; i++) { //can be optimized more?
		if (g_entities[i].inuse && g_entities[i].s.eType == ET_MISSILE && (g_entities[i].r.ownerNum == ent->s.number)) { //Delete (rocket) if its ours
			G_FreeEntity(&g_entities[i]);
			//trap_Print("This only sometimes prints.. even if we have a missile in the air.  (its num: %i, our num: %i, weap type: %i) \n", hit->r.ownerNum, ent->s.number, hit->s.weapon);
		}
	}
}
//sql shit
//void G_UpdatePlaytime(int null, char *username, int seconds);
void ResetPlayerTimers(gentity_t *ent, qboolean print)
{
	qboolean wasReset = qfalse;;

	if (!ent->client)
		return;
	if (ent->client->pers.stats.startTime || ent->client->pers.stats.startTimeFlag)
		wasReset = qtrue;

	if (ent->client->sess.raceMode) {
		VectorClear(ent->client->ps.velocity); //lel
		ent->client->ps.duelTime = 0;
		ent->client->ps.stats[STAT_RESTRICTIONS] = 0; //meh
													  //if (ent->client->ps.fd.forcePowerLevel[FP_LEVITATION] == 3) { //this is a sad hack..
		if (!ent->client->pers.practice) {
			ent->client->ps.powerups[PW_YSALAMIRI] = 0; //beh, only in racemode so wont fuck with ppl using amtele as checkpoints midcourse
		}
		ent->client->ps.powerups[PW_FORCE_BOON] = 0;
		ent->client->pers.haste = qfalse;
		if (ent->health > 0) {
			ent->client->ps.fd.forcePower = 100; //Reset their force back to full i guess!
			ent->client->ps.stats[STAT_HEALTH] = ent->health = 100;
			ent->client->ps.stats[STAT_ARMOR] = 25;
		}
		//}
		if (ent->client->sess.movementStyle == MV_RJQ3 || ent->client->sess.movementStyle == MV_RJCPM) { //Get rid of their rockets when they tele/noclip..? Do this for every style..
			DeletePlayerProjectiles(ent);
		}

		/* //already done every frame ?
		#if _GRAPPLE
		if (ent->client->sess.movementStyle == MV_SLICK && ent->client->hook)
		Weapon_HookFree(ent->client->hook);
		#endif
		*/
		if (ent->client->sess.movementStyle == MV_SPEED) {
			ent->client->ps.fd.forcePower = 50;
		}

		/*if (ent->client->sess.movementStyle == MV_JETPACK) {
			ent->client->ps.jetpackFuel = 100;
			ent->client->ps.eFlags &= ~EF_JETPACK_ACTIVE;
		}*/

		if (ent->client->pers.userName && ent->client->pers.userName[0]) {
			if (ent->client->sess.raceMode && !ent->client->pers.practice && ent->client->pers.stats.startTime) {
				ent->client->pers.stats.racetime += (trap_Milliseconds() - ent->client->pers.stats.startTime)*0.001f - ent->client->afkDuration*0.001f;
				ent->client->afkDuration = 0;
			}
			if (ent->client->pers.stats.racetime > 120.0f) {
				//sql shit
				//G_UpdatePlaytime(0, ent->client->pers.userName, (int)(ent->client->pers.stats.racetime + 0.5f));
				ent->client->pers.stats.racetime = 0.0f;
			}
		}
	}

	ent->client->pers.stats.startLevelTime = 0;
	ent->client->pers.stats.startTime = 0;
	ent->client->pers.stats.topSpeed = 0;
	ent->client->pers.stats.displacement = 0;
	ent->client->pers.stats.displacementSamples = 0;
	ent->client->pers.stats.startTimeFlag = 0;
	ent->client->pers.stats.topSpeedFlag = 0;
	ent->client->pers.stats.displacementFlag = 0;
	ent->client->pers.stats.displacementFlagSamples = 0;
	ent->client->ps.stats[STAT_JUMPTIME] = 0;

	ent->client->pers.stats.lastResetTime = level.time; //well im just not sure

	if (wasReset && print)
		//trap_SendServerCommand( ent-g_entities, "print \"Timer reset!\n\""); //console spam is bad
		trap_SendServerCommand(ent - g_entities, "cp \"Timer reset!\n\n\n\n\n\n\n\n\n\n\n\n\"");
}

/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f(gentity_t *ent) {
	int allowed;
	if (ent->client && ent->client->ps.duelInProgress && ent->client->pers.lastUserName[0]) {
		gentity_t *duelAgainst = &g_entities[ent->client->ps.duelIndex];
		if (duelAgainst->client && duelAgainst->client->pers.lastUserName[0]) {
			trap_SendServerCommand(ent - g_entities, va("print \"You are not authorized to use this command (noclip) in ranked duels.\n\""));
			return; //Dont allow noclip in ranked duels ever
		}
	}

	allowed = G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_NOCLIP, qtrue, jp_allowRaceTele.integer > 1, "noclip");

	if (allowed == 3) { //Admin
		if (trap_Argc() == 2) {
			char client[MAX_NETNAME];
			int clientid;
			gentity_t *target = NULL;

			trap_Argv(1, client, sizeof(client));
			clientid = JP_ClientNumberFromString(ent, client);
			if (clientid == -1 || clientid == -2)
				return;
			target = &g_entities[clientid];
			if (!target->client)
				return;
			trap_SendServerCommand(target - g_entities, va("print \"%s\n\"", target->client->noclip ? "noclip OFF" : "noclip ON"));
			if (target->client->sess.raceMode && target->client->noclip)
				AmTeleportPlayer(target, target->client->ps.origin, target->client->ps.viewangles, qtrue, qtrue, qfalse); //Good
			target->client->noclip = !target->client->noclip;
			if (!sv_cheats.integer)
				ResetPlayerTimers(target, qtrue);
			return;
		}
		if (trap_Argc() == 1) {
			trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", ent->client->noclip ? "noclip OFF" : "noclip ON"));
			if (ent->client->sess.raceMode && ent->client->noclip)
				AmTeleportPlayer(ent, ent->client->ps.origin, ent->client->ps.viewangles, qtrue, qtrue, qfalse); //Good
			ent->client->noclip = !ent->client->noclip;
			if (!sv_cheats.integer)
				ResetPlayerTimers(ent, qtrue);
			return;
		}
	}
	else if (allowed == 2) { //Race only
		trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", ent->client->noclip ? "noclip OFF" : "noclip ON"));
		if (ent->client->sess.raceMode && ent->client->noclip)
			AmTeleportPlayer(ent, ent->client->ps.origin, ent->client->ps.viewangles, qtrue, qtrue, qfalse); //Good
		ent->client->noclip = !ent->client->noclip;
		if (!sv_cheats.integer)
			ResetPlayerTimers(ent, qtrue);
	}
	else if (allowed) { //Cheats enabled only
		trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", ent->client->noclip ? "noclip OFF" : "noclip ON"));
		ent->client->noclip = !ent->client->noclip;
	}
}


/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void Cmd_LevelShot_f( gentity_t *ent ) {
	if ( !CheatsOk( ent ) ) {
		return;
	}

	// doesn't work in single player
	if ( g_gametype.integer != 0 ) {
		trap_SendServerCommand( ent-g_entities, 
			"print \"Must be in g_gametype 0 for levelshot\n\"" );
		return;
	}

	BeginIntermission();
	trap_SendServerCommand( ent-g_entities, "clientLevelShot" );
}


/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void Cmd_TeamTask_f( gentity_t *ent ) {
	char userinfo[MAX_INFO_STRING];
	char		arg[MAX_TOKEN_CHARS];
	int task;
	int client = ent->client - level.clients;

	if ( trap_Argc() != 2 ) {
		return;
	}
	trap_Argv( 1, arg, sizeof( arg ) );
	task = atoi( arg );

	trap_GetUserinfo(client, userinfo, sizeof(userinfo));
	Info_SetValueForKey(userinfo, "teamtask", va("%d", task));
	trap_SetUserinfo(client, userinfo);
	ClientUserinfoChanged(client);
}



/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f( gentity_t *ent ) {
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}
	if (ent->health <= 0) {
		return;
	}

	if (g_gametype.integer == GT_TOURNAMENT && level.numPlayingClients > 1 && !level.warmupTime)
	{
		if (!g_allowDuelSuicide.integer)
		{
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "ATTEMPTDUELKILL")) );
			return;
		}
	}

	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
	player_die (ent, ent, ent, 100000, MOD_SUICIDE);
}

gentity_t *G_GetDuelWinner(gclient_t *client)
{
	gclient_t *wCl;
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		wCl = &level.clients[i];
		
		if (wCl && wCl != client && /*wCl->ps.clientNum != client->ps.clientNum &&*/
			wCl->pers.connected == CON_CONNECTED && wCl->sess.sessionTeam != TEAM_SPECTATOR)
		{
			return &g_entities[wCl->ps.clientNum];
		}
	}

	return NULL;
}

/*
=================
BroadCastTeamChange

Let everyone know about a team change
=================
*/
void BroadcastTeamChange( gclient_t *client, int oldTeam )
{
	client->ps.fd.forceDoInit = 1; //every time we change teams make sure our force powers are set right

	if ( client->sess.sessionTeam == TEAM_RED ) {
		trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
			client->pers.netname, G_GetStripEdString("SVINGAME", "JOINEDTHEREDTEAM")) );
	} else if ( client->sess.sessionTeam == TEAM_BLUE ) {
		trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
		client->pers.netname, G_GetStripEdString("SVINGAME", "JOINEDTHEBLUETEAM")));
	} else if ( client->sess.sessionTeam == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR ) {
		trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
		client->pers.netname, G_GetStripEdString("SVINGAME", "JOINEDTHESPECTATORS")));
	} else if ( client->sess.sessionTeam == TEAM_FREE ) {
		if (g_gametype.integer == GT_TOURNAMENT)
		{
			/*
			gentity_t *currentWinner = G_GetDuelWinner(client);

			if (currentWinner && currentWinner->client)
			{
				trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s %s\n\"",
				currentWinner->client->pers.netname, G_GetStripEdString("SVINGAME", "VERSUS"), client->pers.netname));
			}
			else
			{
				trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
				client->pers.netname, G_GetStripEdString("SVINGAME", "JOINEDTHEBATTLE")));
			}
			*/
			//NOTE: Just doing a vs. once it counts two players up
		}
		else
		{
			trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
			client->pers.netname, G_GetStripEdString("SVINGAME", "JOINEDTHEBATTLE")));
		}
	}

	G_LogPrintf ( "setteam:  %i %s %s\n",
				  (int)(client - &level.clients[0]),
				  TeamName ( oldTeam ),
				  TeamName ( client->sess.sessionTeam ) );
}


static int GetTeamPlayers(int team) {
	int i, count = 0;
	gclient_t        *cl;

	for (i = 0; i<MAX_CLIENTS; i++) {//Build a list of clients.. sv_maxclients? w/e
		if (!g_entities[i].inuse)
			continue;
		cl = &level.clients[i];
		if (team == TEAM_RED && cl->sess.sessionTeam == TEAM_RED) {
			count++;
		}
		else if (team == TEAM_BLUE && cl->sess.sessionTeam == TEAM_BLUE) {
			count++;
		}
		else if (team == TEAM_FREE && cl->sess.sessionTeam == TEAM_FREE) {
			count++;
		}
	}
	return count;
}
/*
=================
SetTeam
=================
*/
qboolean g_dontPenalizeTeam = qfalse;
qboolean g_preventTeamBegin = qfalse;
void SetTeam(gentity_t *ent, char *s, qboolean forcedToJoin) {//JAPRO - Modified for proper amforceteam.  Why doesn't this accept TEAM_FREE and stuff and instead use char??
	int					team, oldTeam;
	gclient_t			*client;
	int					clientNum;
	spectatorState_t	specState;
	int					specClient;
	int					teamLeader;

	// fix: this prevents rare creation of invalid players
	if (!ent->inuse)
	{
		return;
	}

	//
	// see what change is requested
	//
	client = ent->client;

	clientNum = client - level.clients;
	specClient = 0;
	specState = SPECTATOR_NOT;
	if (!Q_stricmp(s, "scoreboard") || !Q_stricmp(s, "score")) {
		if (level.isLockedspec && !forcedToJoin)
		{
			trap_SendServerCommand(ent->client->ps.clientNum, va("print \"^7The ^3Spectator ^7Access has been locked!\n\""));
			return;
		}
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE; // SPECTATOR_SCOREBOARD disabling this for now since it is totally broken on client side
	}
	else if (!Q_stricmp(s, "follow1")) {
		if (level.isLockedspec && !forcedToJoin)
		{
			trap_SendServerCommand(ent->client->ps.clientNum, va("print \"^7The ^3Spectator ^7Access has been locked!\n\""));
			return;
		}
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -1;
	}
	else if (!Q_stricmp(s, "follow2")) {
		if (level.isLockedspec && !forcedToJoin)
		{
			trap_SendServerCommand(ent->client->ps.clientNum, va("print \"^7The ^3Spectator ^7Access has been locked!\n\""));
			return;
		}
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -2;
	}
	else if (!Q_stricmp(s, "spectator") || !Q_stricmp(s, "s") || !Q_stricmp(s, "spectate")) {
		if (level.isLockedspec && !forcedToJoin)
		{
			trap_SendServerCommand(ent->client->ps.clientNum, va("print \"^7The ^3Spectator ^7Access has been locked!\n\""));
			return;
		}
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE;
	}
	else if (g_gametype.integer >= GT_TEAM) {
		// if running a team game, assign player to one of the teams
		specState = SPECTATOR_NOT;
		if (!Q_stricmp(s, "red") || !Q_stricmp(s, "r"))
		{
			if (level.isLockedred && !forcedToJoin)
			{
				trap_SendServerCommand(ent->client->ps.clientNum, va("print \"^7The ^1Red ^7team is locked!\n\""));
				return;
			}
			if (sv_maxTeamSize.integer && !forcedToJoin && GetTeamPlayers(TEAM_RED) >= sv_maxTeamSize.integer) {
				trap_SendServerCommand(ent->client->ps.clientNum, va("print \"^7The ^1Red ^7team is full!\n\""));
				return;
			}
			if (jp_forceLogin.integer && !forcedToJoin && !(ent->r.svFlags & SVF_BOT) && !ent->client->pers.userName[0]) {
				trap_SendServerCommand(ent - g_entities, "print \"^1You must login to join the game!\n\"");
				return;
			}
			team = TEAM_RED;
		}
		else if (!Q_stricmp(s, "blue") || !Q_stricmp(s, "b"))
		{
			if (level.isLockedblue && !forcedToJoin)
			{
				trap_SendServerCommand(ent->client->ps.clientNum, va("print \"^7The ^4Blue ^7team is locked!\n\""));
				return;
			}
			if (sv_maxTeamSize.integer && !forcedToJoin && GetTeamPlayers(TEAM_BLUE) >= sv_maxTeamSize.integer) {
				trap_SendServerCommand(ent->client->ps.clientNum, va("print \"^7The ^4Blue ^7team is full!\n\""));
				return;
			}
			if (jp_forceLogin.integer && !forcedToJoin && !(ent->r.svFlags & SVF_BOT) && !ent->client->pers.userName[0]) {
				trap_SendServerCommand(ent - g_entities, "print \"^1You must login to join the game!\n\"");
				return;
			}
			team = TEAM_BLUE;
		}
		else {
			// pick the team with the least number of players
			//For now, don't do this. The legalize function will set powers properly now.
			/*
			if (g_forceBasedTeams.integer)
			{
			if (ent->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
			{
			team = TEAM_BLUE;
			}
			else
			{
			team = TEAM_RED;
			}
			}
			else
			{
			*/
			if (jp_raceMode.integer)
				team = TEAM_FREE;
			else
				team = PickTeam(clientNum);

			if (team == TEAM_BLUE && !forcedToJoin) {
				if (level.isLockedblue) {
					trap_SendServerCommand(ent->client->ps.clientNum, va("print \"^7The ^4Blue ^7team is locked!\n\""));
					return;
				}
				if (sv_maxTeamSize.integer && GetTeamPlayers(TEAM_BLUE) >= sv_maxTeamSize.integer) {
					trap_SendServerCommand(ent->client->ps.clientNum, va("print \"^7The ^4Blue ^7team is full!\n\""));
					return;
				}
			}
			else if (team == TEAM_RED && !forcedToJoin) {
				if (level.isLockedred) {
					trap_SendServerCommand(ent->client->ps.clientNum, va("print \"^7The ^1Red ^7team is locked!\n\""));
					return;
				}
				if (sv_maxTeamSize.integer && GetTeamPlayers(TEAM_RED) >= sv_maxTeamSize.integer) {
					trap_SendServerCommand(ent->client->ps.clientNum, va("print \"^7The ^1Red ^7team is full!\n\""));
					return;
				}
			}
		}

		if (g_teamForceBalance.integer) {
			int		counts[TEAM_NUM_TEAMS];

			//JAC: Invalid clientNum was being used
			counts[TEAM_BLUE] = TeamCount(ent - g_entities, TEAM_BLUE);
			counts[TEAM_RED] = TeamCount(ent - g_entities, TEAM_RED);

			// We allow a spread of two
			if (team == TEAM_RED && counts[TEAM_RED] - counts[TEAM_BLUE] > 1) {
				//For now, don't do this. The legalize function will set powers properly now.
				/*
				if (g_forceBasedTeams.integer && ent->client->ps.fd.forceSide == FORCE_DARKSIDE)
				{
				trap_SendServerCommand( ent->client->ps.clientNum,
				va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYRED_SWITCH")) );
				}
				else
				*/
				{
					//JAC: Invalid clientNum was being used
					trap_SendServerCommand(ent - g_entities,
						va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYRED")));
				}
				return; // ignore the request
			}
			if (team == TEAM_BLUE && counts[TEAM_BLUE] - counts[TEAM_RED] > 1) {
				//For now, don't do this. The legalize function will set powers properly now.
				/*
				if (g_forceBasedTeams.integer && ent->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
				{
				trap_SendServerCommand( ent->client->ps.clientNum,
				va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYBLUE_SWITCH")) );
				}
				else
				*/ 
				{
					//JAC: Invalid clientNum was being used
					trap_SendServerCommand(ent - g_entities,
						va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYBLUE")));
				}
				return; // ignore the request
			}

			// It's ok, the team we are switching to has less or same number of players
		}

		//For now, don't do this. The legalize function will set powers properly now.
		/*
		if (g_forceBasedTeams.integer)
		{
		if (team == TEAM_BLUE && ent->client->ps.fd.forceSide != FORCE_LIGHTSIDE)
		{
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "MUSTBELIGHT")) );
		return;
		}
		if (team == TEAM_RED && ent->client->ps.fd.forceSide != FORCE_DARKSIDE)
		{
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "MUSTBEDARK")) );
		return;
		}
		}
		*/
	}
	else {
		if (level.isLockedfree && !forcedToJoin)
		{
			trap_SendServerCommand(ent->client->ps.clientNum, va("print \"^7The ^3Free ^7team is locked!\n\""));
			return;
		}
		if (jp_forceLogin.integer && !(ent->r.svFlags & SVF_BOT) && !forcedToJoin && !ent->client->pers.userName[0]) {
			trap_SendServerCommand(ent - g_entities, "print \"^1You must login to join the game!\n\"");
			return;
		}
		team = TEAM_FREE; // force them to spectators if there aren't any spots free
	}

	oldTeam = client->sess.sessionTeam;

	// override decision if limiting the players
	if ((g_gametype.integer == GT_TOURNAMENT)
		&& level.numNonSpectatorClients >= 2)
	{
		team = TEAM_SPECTATOR;
	}
	else if (g_maxGameClients.integer > 0 &&
		level.numNonSpectatorClients >= g_maxGameClients.integer)
	{
		team = TEAM_SPECTATOR;
	}

	//
	// decide if we will allow the change
	//
	if (team == oldTeam && team != TEAM_SPECTATOR) {
		return;
	}

	//
	// execute the team change
	//

	//If it's siege then show the mission briefing for the team you just joined.
	//	if (g_gametype.integer == GT_SIEGE && team != TEAM_SPECTATOR)
	//	{
	//		trap_SendServerCommand(clientNum, va("sb %i", team));
	//	}

	// if the player was dead leave the body
	if (client->ps.stats[STAT_HEALTH] <= 0 && client->sess.sessionTeam != TEAM_SPECTATOR) {
		MaintainBodyQueue(ent);
	}

	// he starts at 'base'
	client->pers.teamState.state = TEAM_BEGIN;
	if (oldTeam != TEAM_SPECTATOR) {
		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
		g_dontPenalizeTeam = qtrue;
		player_die(ent, ent, ent, 100000, MOD_SUICIDE);
		g_dontPenalizeTeam = qfalse;

	}
	// they go to the end of the line for tournaments
	if (team == TEAM_SPECTATOR && oldTeam != team)
		AddTournamentQueue(client);

	// clear votes if going to spectator (specs can't vote)
	if (team == TEAM_SPECTATOR && !(jp_tweakVote.integer & TV_CLEAR_SPEC_VOTES))
		G_ClearVote(ent);
	// also clear team votes if switching red/blue or going to spec
	G_ClearTeamVote(ent, oldTeam);

	client->sess.sessionTeam = (team_t)team;
	client->sess.spectatorState = specState;
	client->sess.spectatorClient = specClient;

	client->sess.teamLeader = qfalse;
	if (team == TEAM_RED || team == TEAM_BLUE) {
		teamLeader = TeamLeader(team);
		// if there is no team leader or the team leader is a bot and this client is not a bot
		if (teamLeader == -1 || (!(g_entities[clientNum].r.svFlags & SVF_BOT) && (g_entities[teamLeader].r.svFlags & SVF_BOT))) {
			//SetLeader( team, clientNum );
		}
	}
	// make sure there is a team leader on the team the player came from
	if (oldTeam == TEAM_RED || oldTeam == TEAM_BLUE) {
		CheckTeamLeader(oldTeam);
	}

	BroadcastTeamChange(client, oldTeam);

	//make a disappearing effect where they were before teleporting them to the appropriate spawn point,
	//if we were not on the spec team
	if (oldTeam != TEAM_SPECTATOR)
	{
		gentity_t *tent = G_TempEntity(client->ps.origin, EV_PLAYER_TELEPORT_OUT);
		tent->s.clientNum = clientNum;
	}

	// get and distribute relevent paramters
	/*if (!ClientUserinfoChanged(clientNum))
		return;*/

	//sql shit
	/*if (client->ps.duelInProgress) {
		gentity_t *duelAgainst = &g_entities[client->ps.duelIndex];

		if (ent->client->pers.lastUserName && ent->client->pers.lastUserName[0] && duelAgainst->client && duelAgainst->client->pers.lastUserName && duelAgainst->client->pers.lastUserName[0]) {
			if (!(ent->client->sess.accountFlags & JAPRO_ACCOUNTFLAG_NODUEL) && !(duelAgainst->client->sess.accountFlags & JAPRO_ACCOUNTFLAG_NODUEL))
				G_AddDuel(duelAgainst->client->pers.lastUserName, ent->client->pers.lastUserName, duelAgainst->client->pers.duelStartTime, dueltypes[ent->client->ps.clientNum], duelAgainst->client->ps.stats[STAT_HEALTH], duelAgainst->client->ps.stats[STAT_ARMOR]);
		}
	}*/

	ClientUserinfoChanged(clientNum);

	if (!g_preventTeamBegin)
	{
		ClientBegin(clientNum, qfalse);
	}
}

/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
void StopFollowing( gentity_t *ent ) {
	ent->client->ps.persistant[ PERS_TEAM ] = TEAM_SPECTATOR;	
	ent->client->sess.sessionTeam = TEAM_SPECTATOR;	
	ent->client->sess.spectatorState = SPECTATOR_FREE;
	ent->client->ps.pm_flags &= ~PMF_FOLLOW;
	ent->r.svFlags &= ~SVF_BOT;
	ent->client->ps.clientNum = ent - g_entities;
	ent->client->ps.weapon = WP_NONE;
}

/*
=================
Cmd_Team_f
=================
*/
void Cmd_Team_f( gentity_t *ent ) {
	int			oldTeam;
	char		s[MAX_TOKEN_CHARS];

	if ( trap_Argc() != 2 ) {
		oldTeam = ent->client->sess.sessionTeam;
		switch ( oldTeam ) {
		case TEAM_BLUE:
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "PRINTBLUETEAM")) );
			break;
		case TEAM_RED:
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "PRINTREDTEAM")) );
			break;
		case TEAM_FREE:
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "PRINTFREETEAM")) );
			break;
		case TEAM_SPECTATOR:
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "PRINTSPECTEAM")) );
			break;
		}
		return;
	}

	if ( ent->client->switchTeamTime > level.time ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOSWITCH")) );
		return;
	}

	if (gEscaping)
	{
		return;
	}

	// if they are playing a tournement game, count as a loss
	if ( (g_gametype.integer == GT_TOURNAMENT )
		&& ent->client->sess.sessionTeam == TEAM_FREE ) {//in a tournament game
		//disallow changing teams
		trap_SendServerCommand( ent-g_entities, "print \"Cannot switch teams in Duel\n\"" );
		return;
		//FIXME: why should this be a loss???
		//ent->client->sess.losses++;
	}

	trap_Argv( 1, s, sizeof( s ) );

	SetTeam( ent, s, qfalse );

	ent->client->switchTeamTime = level.time + 5000;
}

/*
=================
Cmd_Team_f
=================
*/
void Cmd_ForceChanged_f( gentity_t *ent )
{
	char fpChStr[1024];
	const char *buf;
//	Cmd_Kill_f(ent);
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{ //if it's a spec, just make the changes now
		//trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "FORCEAPPLIED")) );
		//No longer print it, as the UI calls this a lot.
		WP_InitForcePowers( ent );
		goto argCheck;
	}

	buf = G_GetStripEdString("SVINGAME", "FORCEPOWERCHANGED");

	strcpy(fpChStr, buf);

	trap_SendServerCommand( ent-g_entities, va("print \"%s%s\n\n\"", S_COLOR_GREEN, fpChStr) );

	ent->client->ps.fd.forceDoInit = 1;
argCheck:
	if (g_gametype.integer == GT_TOURNAMENT)
	{ //If this is duel, don't even bother changing team in relation to this.
		return;
	}

	if (trap_Argc() > 1)
	{
		char	arg[MAX_TOKEN_CHARS];

		trap_Argv( 1, arg, sizeof( arg ) );

		if ( !Q_stricmp(arg, "none") || !Q_stricmp(arg, "same") || !Q_stricmp(arg, ";") ) return; // 1.02 clients send those and trigger unwanted team-changes...

		//if there's an arg, assume it's a combo team command from the UI.
		Cmd_Team_f(ent);
	}
}

/*
=================
Cmd_Follow_f
=================
*/
void Cmd_Follow_f( gentity_t *ent ) {
	int		i;
	char	arg[MAX_TOKEN_CHARS];
	
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR && ent->client->switchTeamTime > level.time )
	{
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOSWITCH")) );
		return;
	}

	if ( trap_Argc() != 2 ) {
		if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
			StopFollowing( ent );
		}
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	i = ClientNumberFromString( ent, arg );
	if ( i == -1 ) {
		return;
	}

	// can't follow self
	if ( &level.clients[ i ] == ent->client ) {
		return;
	}

	// can't follow another spectator
	if ( level.clients[ i ].sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}

	// if they are playing a tournement game, count as a loss
	if ( (g_gametype.integer == GT_TOURNAMENT )
		&& ent->client->sess.sessionTeam == TEAM_FREE ) {
		//WTF???
		ent->client->sess.losses++;
	}

	// first set them to spectator
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		SetTeam( ent, "spectator", qfalse );
	}

	ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = i;
}

/*
=================
Cmd_FollowCycle_f
=================
*/
void Cmd_FollowCycle_f( gentity_t *ent, int dir ) {
	int		clientnum;
	int		original;
	
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR && ent->client->switchTeamTime > level.time )
	{
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOSWITCH")) );
		return;
	}

	// if they are playing a tournement game, count as a loss
	if ( (g_gametype.integer == GT_TOURNAMENT )
		&& ent->client->sess.sessionTeam == TEAM_FREE ) {\
		//WTF???
		ent->client->sess.losses++;
	}
	// first set them to spectator
	if ( ent->client->sess.spectatorState == SPECTATOR_NOT ) {
		SetTeam( ent, "spectator", qfalse );
	}

	if ( dir != 1 && dir != -1 ) {
		G_Error( "Cmd_FollowCycle_f: bad dir %i", dir );
	}

	clientnum = ent->client->sess.spectatorClient;
	original = clientnum;

	if ( original >= MAX_CLIENTS || original < 0 ) original = 0; // SpectatorCrashFix (infinite loop)

	do {
		clientnum += dir;
		if ( clientnum >= level.maxclients ) {
			clientnum = 0;
		}
		if ( clientnum < 0 ) {
			clientnum = level.maxclients - 1;
		}

		// can only follow connected clients
		if ( level.clients[ clientnum ].pers.connected != CON_CONNECTED ) {
			continue;
		}

		// can't follow another spectator
		if ( level.clients[ clientnum ].sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}

		// this is good, we can use it
		ent->client->sess.spectatorClient = clientnum;
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		return;
	} while ( clientnum != original );

	// leave it where it was
}


/*
==================
G_Say
==================
*/

static void G_SayTo( gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message ) {
	if (!other) {
		return;
	}
	if (!other->inuse) {
		return;
	}
	if (!other->client) {
		return;
	}
	if ( other->client->pers.connected != CON_CONNECTED ) {
		return;
	}
	/*if ( mode == SAY_TEAM  && !OnSameTeam(ent, other) ) {
		return;
	}*/
	if (mode == SAY_TEAM && ((g_gametype.integer >= GT_TEAM && !OnSameTeam(ent, other)) || (g_gametype.integer < GT_TEAM && (ent->client->sess.sessionTeam != other->client->sess.sessionTeam)))) {
		return;
	}

	if (mode == SAY_CLAN && ((Q_stricmp(ent->client->sess.clanpass, other->client->sess.clanpass) || ent->client->sess.clanpass[0] == 0 || other->client->sess.clanpass[0] == 0)))//Idk
		return;//Ignore it
	if (mode == SAY_ADMIN && !G_AdminAllowed(other, JAPRO_ACCOUNTFLAG_A_READAMSAY, qfalse, qfalse, NULL) && ent != other)
		return;

	// no chatting to players in tournements
	/*if ( (g_gametype.integer == GT_TOURNAMENT )
		&& other->client->sess.sessionTeam == TEAM_FREE
		&& ent->client->sess.sessionTeam != TEAM_FREE ) {
		//Hmm, maybe some option to do so if allowed?  Or at least in developer mode...
		return;
	}*/

	trap_SendServerCommand( other-g_entities, va("%s \"%s%c%c%s\"", 
		mode == SAY_TEAM ? "tchat" : "chat",
		name, Q_COLOR_ESCAPE, color, message));
}

#define EC		"\x19"

void G_Say( gentity_t *ent, gentity_t *target, int mode, const char *chatText ) {
	int			j;
	gentity_t	*other;
	int			color;
	char		name[64];
	// don't let text be too long for malicious reasons
	char		text[MAX_SAY_TEXT];
	char		location[64];

	if ( g_gametype.integer < GT_TEAM && mode == SAY_TEAM ) {
		mode = SAY_ALL;
	}

	switch ( mode ) {
	default:
	case SAY_ALL:
		G_LogPrintf( "say: %s: %s\n", ent->client->pers.netname, chatText );
		Com_sprintf (name, sizeof(name), "%s%c%c"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_GREEN;
		break;
	case SAY_TEAM:
		G_LogPrintf( "sayteam: %s: %s\n", ent->client->pers.netname, chatText );
		if (Team_GetLocationMsg(ent, location, sizeof(location)))
			Com_sprintf (name, sizeof(name), EC"(%s%c%c"EC") (%s)"EC": ", 
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location);
		else
			Com_sprintf (name, sizeof(name), EC"(%s%c%c"EC")"EC": ", 
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_CYAN;
		break;
	case SAY_TELL:
		if (target && g_gametype.integer >= GT_TEAM &&
			target->client->sess.sessionTeam == ent->client->sess.sessionTeam &&
			Team_GetLocationMsg(ent, location, sizeof(location)))
			Com_sprintf (name, sizeof(name), EC"[%s%c%c"EC"] (%s)"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location );
		else
			Com_sprintf (name, sizeof(name), EC"[%s%c%c"EC"]"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_MAGENTA;
		break;
	}

	Q_strncpyz( text, chatText, sizeof(text) );

	if ( target ) {
		G_SayTo( ent, target, mode, color, name, text );
		return;
	}

	// echo the text to the console
	if ( g_dedicated.integer ) {
		G_Printf( "%s%s\n", name, text);
	}

	// send it to all the apropriate clients
	for (j = 0; j < level.maxclients; j++) {
		other = &g_entities[j];
		G_SayTo( ent, other, mode, color, name, text );
	}
}


/*
==================
Cmd_Say_f
==================
*/
static void Cmd_Say_f( gentity_t *ent, int mode, qboolean arg0 ) {
	char		*p;

	if ( trap_Argc () < 2 && !arg0 ) {
		return;
	}

	if (arg0)
	{
		p = ConcatArgs( 0 );
	}
	else
	{
		p = ConcatArgs( 1 );
	}

	G_Say( ent, NULL, mode, p );
}

/*
==================
Cmd_Tell_f
==================
*/
static void Cmd_Tell_f( gentity_t *ent ) {
	int			targetNum;
	gentity_t	*target;
	char		*p;
	char		arg[MAX_TOKEN_CHARS];

	if ( trap_Argc () < 2 ) {
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	targetNum = atoi( arg );
	if ( targetNum < 0 || targetNum >= level.maxclients ) {
		return;
	}

	target = &g_entities[targetNum];
	if ( !target || !target->inuse || !target->client ) {
		return;
	}

	p = ConcatArgs( 2 );

	G_LogPrintf( "tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, p );
	G_Say( ent, target, SAY_TELL, p );
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if ( ent != target && !(ent->r.svFlags & SVF_BOT)) {
		G_Say( ent, ent, SAY_TELL, p );
	}
}


static void G_VoiceTo( gentity_t *ent, gentity_t *other, int mode, const char *id, qboolean voiceonly ) {
	int color;
	char *cmd;

	if (!other) {
		return;
	}
	if (!other->inuse) {
		return;
	}
	if (!other->client) {
		return;
	}
	if ( mode == SAY_TEAM && !OnSameTeam(ent, other) ) {
		return;
	}
	// no chatting to players in tournements
	if (g_gametype.integer == GT_TOURNAMENT) {
		return;
	}

	if (mode == SAY_TEAM) {
		color = COLOR_CYAN;
		cmd = "vtchat";
	}
	else if (mode == SAY_TELL) {
		color = COLOR_MAGENTA;
		cmd = "vtell";
	}
	else {
		color = COLOR_GREEN;
		cmd = "vchat";
	}

	trap_SendServerCommand( other-g_entities, va("%s %d %d %d %s", cmd, voiceonly, ent->s.number, color, id));
}

void G_Voice( gentity_t *ent, gentity_t *target, int mode, const char *id, qboolean voiceonly ) {
	int			j;
	gentity_t	*other;

	if ( g_gametype.integer < GT_TEAM && mode == SAY_TEAM ) {
		mode = SAY_ALL;
	}

	if ( target ) {
		G_VoiceTo( ent, target, mode, id, voiceonly );
		return;
	}

	// echo the text to the console
	if ( g_dedicated.integer ) {
		G_Printf( "voice: %s %s\n", ent->client->pers.netname, id);
	}

	// send it to all the apropriate clients
	for (j = 0; j < level.maxclients; j++) {
		other = &g_entities[j];
		G_VoiceTo( ent, other, mode, id, voiceonly );
	}
}

#if 0
/*
==================
Cmd_Voice_f
==================
*/
static void Cmd_Voice_f( gentity_t *ent, int mode, qboolean arg0, qboolean voiceonly ) {
	char		*p;

	if ( trap_Argc () < 2 && !arg0 ) {
		return;
	}

	if (arg0)
	{
		p = ConcatArgs( 0 );
	}
	else
	{
		p = ConcatArgs( 1 );
	}

	G_Voice( ent, NULL, mode, p, voiceonly );
}

/*
==================
Cmd_VoiceTell_f
==================
*/
static void Cmd_VoiceTell_f( gentity_t *ent, qboolean voiceonly ) {
	int			targetNum;
	gentity_t	*target;
	char		*id;
	char		arg[MAX_TOKEN_CHARS];

	if ( trap_Argc () < 2 ) {
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	targetNum = atoi( arg );
	if ( targetNum < 0 || targetNum >= level.maxclients ) {
		return;
	}

	target = &g_entities[targetNum];
	if ( !target || !target->inuse || !target->client ) {
		return;
	}

	id = ConcatArgs( 2 );

	G_LogPrintf( "vtell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, id );
	G_Voice( ent, target, SAY_TELL, id, voiceonly );
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if ( ent != target && !(ent->r.svFlags & SVF_BOT)) {
		G_Voice( ent, ent, SAY_TELL, id, voiceonly );
	}
}


/*
==================
Cmd_VoiceTaunt_f
==================
*/
static void Cmd_VoiceTaunt_f( gentity_t *ent ) {
	gentity_t *who;
	int i;

	if (!ent->client) {
		return;
	}

	// insult someone who just killed you
	if (ent->enemy && ent->enemy->client && ent->enemy->client->lastkilled_client == ent->s.number) {
		// i am a dead corpse
		if (!(ent->enemy->r.svFlags & SVF_BOT)) {
			G_Voice( ent, ent->enemy, SAY_TELL, VOICECHAT_DEATHINSULT, qfalse );
		}
		if (!(ent->r.svFlags & SVF_BOT)) {
			G_Voice( ent, ent,        SAY_TELL, VOICECHAT_DEATHINSULT, qfalse );
		}
		ent->enemy = NULL;
		return;
	}
	// insult someone you just killed
	if (ent->client->lastkilled_client >= 0 && ent->client->lastkilled_client != ent->s.number) {
		who = g_entities + ent->client->lastkilled_client;
		if (who->client) {
			// who is the person I just killed
			if (who->client->lasthurt_mod == MOD_STUN_BATON) {
				if (!(who->r.svFlags & SVF_BOT)) {
					G_Voice( ent, who, SAY_TELL, VOICECHAT_KILLGAUNTLET, qfalse );	// and I killed them with a gauntlet
				}
				if (!(ent->r.svFlags & SVF_BOT)) {
					G_Voice( ent, ent, SAY_TELL, VOICECHAT_KILLGAUNTLET, qfalse );
				}
			} else {
				if (!(who->r.svFlags & SVF_BOT)) {
					G_Voice( ent, who, SAY_TELL, VOICECHAT_KILLINSULT, qfalse );	// and I killed them with something else
				}
				if (!(ent->r.svFlags & SVF_BOT)) {
					G_Voice( ent, ent, SAY_TELL, VOICECHAT_KILLINSULT, qfalse );
				}
			}
			ent->client->lastkilled_client = -1;
			return;
		}
	}

	if (g_gametype.integer >= GT_TEAM) {
		// praise a team mate who just got a reward
		for(i = 0; i < MAX_CLIENTS; i++) {
			who = g_entities + i;
			if (who->client && who != ent && who->client->sess.sessionTeam == ent->client->sess.sessionTeam) {
				if (who->client->rewardTime > level.time) {
					if (!(who->r.svFlags & SVF_BOT)) {
						G_Voice( ent, who, SAY_TELL, VOICECHAT_PRAISE, qfalse );
					}
					if (!(ent->r.svFlags & SVF_BOT)) {
						G_Voice( ent, ent, SAY_TELL, VOICECHAT_PRAISE, qfalse );
					}
					return;
				}
			}
		}
	}

	// just say something
	G_Voice( ent, NULL, SAY_ALL, VOICECHAT_TAUNT, qfalse );
}
#endif


static char	*gc_orders[] = {
	"hold your position",
	"hold this position",
	"come here",
	"cover me",
	"guard location",
	"search and destroy",
	"report"
};

void Cmd_GameCommand_f( gentity_t *ent ) {
	int		player;
	int		order;
	char	str[MAX_TOKEN_CHARS];

	trap_Argv( 1, str, sizeof( str ) );
	player = atoi( str );
	trap_Argv( 2, str, sizeof( str ) );
	order = atoi( str );

	if ( player < 0 || player >= MAX_CLIENTS ) {
		return;
	}
	if ( order < 0 || order > (int)ARRAY_LEN(gc_orders) ) {
		return;
	}
	G_Say( ent, &g_entities[player], SAY_TELL, gc_orders[order] );
	G_Say( ent, ent, SAY_TELL, gc_orders[order] );
}

/*
==================
Cmd_Where_f
==================
*/
void Cmd_Where_f( gentity_t *ent ) {
	trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", vtos( ent->s.origin ) ) );
}

static const char *gameNames[] = {
	"Free For All",
	"Holocron FFA",
	"Jedi Master",
	"Duel",
	"Single Player",
	"Team FFA",
	"N/A",
	"Capture the Flag",
	"Capture the Ysalamiri"
};

/*
==================
G_ClientNumberFromName

Finds the client number of the client with the given name
==================
*/
int G_ClientNumberFromName ( const char* name )
{
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];
	int			i;
	gclient_t*	cl;

	// check for a name match
	SanitizeString( (char*)name, s2 );
	for ( i=0, cl=level.clients ; i < level.numConnectedClients ; i++, cl++ ) 
	{
		SanitizeString( cl->pers.netname, n2 );
		if ( !strcmp( n2, s2 ) ) 
		{
			return i;
		}
	}

	return -1;
}

/*
==================
G_ClientNumberFromStrippedName

Same as above, but strips special characters out of the names before comparing.
==================
*/
int G_ClientNumberFromStrippedName ( const char* name )
{
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];
	int			i;
	gclient_t*	cl;

	// check for a name match
	SanitizeString2( (char*)name, s2 );
	for ( i=0, cl=level.clients ; i < level.numConnectedClients ; i++, cl++ ) 
	{
		SanitizeString2( cl->pers.netname, n2 );
		if ( !strcmp( n2, s2 ) ) 
		{
			return i;
		}
	}

	return -1;
}

const char *G_GetArenaInfoByMap(const char *map);

void Cmd_MapList_f(gentity_t *ent) {
	int i, toggle = 0;
	char map[24] = "--", buf[512] = { 0 };

	Q_strcat(buf, sizeof(buf), "Map list:");

	for (i = 0; i<level.arenas.num; i++) {
		Q_strncpyz(map, Info_ValueForKey(level.arenas.infos[i], "map"), sizeof(map));
		Q_StripColor(map);

		if (G_DoesMapSupportGametype(map, g_gametype.integer)) {
			char *tmpMsg = va(" ^%c%s", (++toggle & 1) ? COLOR_GREEN : COLOR_YELLOW, map);
			if (strlen(buf) + strlen(tmpMsg) >= sizeof(buf)) {
				trap_SendServerCommand(ent - g_entities, va("print \"%s\"", buf));
				buf[0] = '\0';
			}
			Q_strcat(buf, sizeof(buf), tmpMsg);
		}
	}

	trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", buf));
}

qboolean G_VoteCapturelimit(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	int n = Com_Clampi(0, 0x7FFFFFFF, atoi(arg2));
	Com_sprintf(level.voteString, sizeof(level.voteString), "%s %i", arg1, n);
	Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "%s", level.voteString);
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VoteClientkick(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	int n = atoi(arg2);

	if (n < 0 || n >= level.maxclients) {
		trap_SendServerCommand(ent - g_entities, va("print \"invalid client number %d.\n\"", n));
		return qfalse;
	}

	if (g_entities[n].client->pers.connected == CON_DISCONNECTED) {
		trap_SendServerCommand(ent - g_entities, va("print \"there is no client with the client number %d.\n\"", n));
		return qfalse;
	}

	Com_sprintf(level.voteString, sizeof(level.voteString), "%s %s", arg1, arg2);
	Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "%s %s", arg1, g_entities[n].client->pers.netname);
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VoteFraglimit(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	int n = Com_Clampi(0, 0x7FFFFFFF, atoi(arg2));
	Com_sprintf(level.voteString, sizeof(level.voteString), "%s %i", arg1, n);
	Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "%s", level.voteString);
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VoteGametype(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	int gt = atoi(arg2);

	// ffa, ctf, tdm, etc
	//if (arg2[0] && isalpha(arg2[0])) {
	if (arg2[0] && ((arg2[0] >= 'a' && arg2[0] <= 'z') || (arg2[0] >= 'A' && arg2[0] <= 'Z'))) {
		gt = BG_GetGametypeForString(arg2);
		if (gt == -1)
		{
			trap_SendServerCommand(ent - g_entities, va("print \"Gametype (%s) unrecognised, defaulting to FFA/Deathmatch\n\"", arg2));
			gt = GT_FFA;
		}
	}
	// numeric but out of range
	else if (gt < 0 || gt >= GT_MAX_GAME_TYPE) {
		trap_SendServerCommand(ent - g_entities, va("print \"Gametype (%i) is out of range, defaulting to FFA/Deathmatch\n\"", gt));
		gt = GT_FFA;
	}

	// logically invalid gametypes, or gametypes not fully implemented in MP
	if (gt == GT_SINGLE_PLAYER) {
		trap_SendServerCommand(ent - g_entities, va("print \"This gametype is not supported (%s).\n\"", arg2));
		return qfalse;
	}

	level.votingGametype = qtrue;
	level.votingGametypeTo = gt;

	Com_sprintf(level.voteString, sizeof(level.voteString), "%s %d", arg1, gt);
	Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "%s %s", arg1, gameNames[gt]);
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));

	return qtrue;
}

qboolean G_VoteKick(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	int clientid = JP_ClientNumberFromString(ent, arg2);
	gentity_t *target = NULL;

	if (clientid == -1 || clientid == -2)
		return qfalse;

	target = &g_entities[clientid];
	if (!target || !target->inuse || !target->client)
		return qfalse;

	Com_sprintf(level.voteString, sizeof(level.voteString), "clientkick %d", clientid);
	Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "kick %s", target->client->pers.netname);
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VoteMap(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	char s[MAX_CVAR_VALUE_STRING] = { 0 }, bspName[MAX_QPATH] = { 0 }, *mapName = NULL, *mapName2 = NULL;
	fileHandle_t fp = NULL_FILE;
	const char *arenaInfo;

	// didn't specify a map, show available maps
	if (numArgs < 3) {
		Cmd_MapList_f(ent);
		return qfalse;
	}

	if (strchr(arg2, '\\')) {
		trap_SendServerCommand(ent - g_entities, "print \"Can't have mapnames with a \\\n\"");
		return qfalse;
	}

	Com_sprintf(bspName, sizeof(bspName), "maps/%s.bsp", arg2);
	if (trap_FS_FOpenFile(bspName, &fp, FS_READ) <= 0) {
		trap_SendServerCommand(ent - g_entities, va("print \"Can't find map %s on server\n\"", bspName));
		if (fp != NULL_FILE)
			trap_FS_FCloseFile(fp);
		return qfalse;
	}
	trap_FS_FCloseFile(fp);

	if (!G_DoesMapSupportGametype(arg2, g_gametype.integer) /*&& !(g_tweakVote.integer & TV_FIX_GAMETYPEMAP)*/) { //new TV for check arena file for matching gametype?
																											  //Logic, this is not needed because we have live update gametype?
		trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTE_MAPNOTSUPPORTEDBYGAME")));
		return qfalse;
	}

	// preserve the map rotation
	trap_Cvar_VariableStringBuffer("nextmap", s, sizeof(s));
	if (*s)
		Com_sprintf(level.voteString, sizeof(level.voteString), "%s %s; set nextmap \"%s\"", arg1, arg2, s);
	else
		Com_sprintf(level.voteString, sizeof(level.voteString), "%s %s", arg1, arg2);

	arenaInfo = G_GetArenaInfoByMap(arg2);
	if (arenaInfo) {
		mapName = Info_ValueForKey(arenaInfo, "longname");
		mapName2 = Info_ValueForKey(arenaInfo, "map");
	}

	if (!mapName || !mapName[0])
		mapName = "ERROR";

	if (!mapName2 || !mapName2[0])
		mapName2 = "ERROR";

	Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "map %s (%s)", mapName, mapName2);
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VoteMapRestart(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	int n = Com_Clampi(0, 60, atoi(arg2));
	if (numArgs < 3)
		n = 5;
	Com_sprintf(level.voteString, sizeof(level.voteString), "%s %i", arg1, n);
	Q_strncpyz(level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString));
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VoteNextmap(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	char s[MAX_CVAR_VALUE_STRING];

	trap_Cvar_VariableStringBuffer("nextmap", s, sizeof(s));
	if (!*s) {
		trap_SendServerCommand(ent - g_entities, "print \"nextmap not set.\n\"");
		return qfalse;
	}
	//SiegeClearSwitchData();
	Com_sprintf(level.voteString, sizeof(level.voteString), "vstr nextmap");
	Q_strncpyz(level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString));
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VoteTimelimit(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	float tl = Com_Clamp(0.0f, 35790.0f, atof(arg2));
	if (Q_isintegral(tl))
		Com_sprintf(level.voteString, sizeof(level.voteString), "%s %i", arg1, (int)tl);
	else
		Com_sprintf(level.voteString, sizeof(level.voteString), "%s %.3f", arg1, tl);
	Q_strncpyz(level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString));
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VoteWarmup(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	int n = Com_Clampi(0, 1, atoi(arg2));
	Com_sprintf(level.voteString, sizeof(level.voteString), "%s %i", arg1, n);
	Q_strncpyz(level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString));
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VoteTeamSize(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	int ts = Com_Clampi(0, 32, atof(arg2)); //uhhh... k


	Com_sprintf(level.voteString, sizeof(level.voteString), "%s %i", arg1, ts);

	Q_strncpyz(level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString));
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VoteVSTR(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	char vstr[64] = { 0 };
	char buf[MAX_CVAR_VALUE_STRING];

	Q_strncpyz(vstr, arg2, sizeof(vstr));
	//clean the string?
	Q_strlwr(vstr);
	Q_CleanStr(vstr, qfalse);

	//Check if vstr exists, if not return qfalse.
	trap_Cvar_VariableStringBuffer(vstr, buf, sizeof(buf));
	if (!Q_stricmp(buf, ""))
		return qfalse;

	Com_sprintf(level.voteString, sizeof(level.voteString), "%s %s", arg1, vstr);

	Q_strncpyz(level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString));
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VotePoll(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	char question[64] = { 0 };

	Q_strncpyz(question, arg2, sizeof(question));
	//clean the string?
	Q_strlwr(question);
	Q_CleanStr(question, qfalse);

	Com_sprintf(level.voteString, sizeof(level.voteString), "");

	Q_strncpyz(level.voteDisplayString, va("poll: %s", question), sizeof(level.voteDisplayString));
	Q_strncpyz(level.voteStringClean, va("poll: %s", question), sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VotePause(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {

	Com_sprintf(level.voteString, sizeof(level.voteString), "pause");
	Q_strncpyz(level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString));
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VoteReset(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {

	Com_sprintf(level.voteString, sizeof(level.voteString), "resetScores");
	Q_strncpyz(level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString));
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VoteForceSpec(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	int n = atoi(arg2);

	if (n < 0 || n >= level.maxclients) {
		trap_SendServerCommand(ent - g_entities, va("print \"invalid client number %d.\n\"", n));
		return qfalse;
	}

	if (g_entities[n].client->pers.connected == CON_DISCONNECTED) {
		trap_SendServerCommand(ent - g_entities, va("print \"there is no client with the client number %d.\n\"", n));
		return qfalse;
	}

	Com_sprintf(level.voteString, sizeof(level.voteString), "forceteam %i s", n);
	Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "forcespec %s", g_entities[n].client->pers.netname);
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

typedef struct voteString_s {
	const char	*string;
	const char	*aliases;	// space delimited list of aliases, will always show the real vote string
	qboolean(*func)(gentity_t *ent, int numArgs, const char *arg1, const char *arg2);
	int			numArgs;	// number of REQUIRED arguments, not total/optional arguments
	uint32_t	validGT;	// bit-flag of valid gametypes
	qboolean	voteDelay;	// if true, will delay executing the vote string after it's accepted by g_voteDelay
	const char	*shortHelp;	// NULL if no arguments needed
} voteString_t;

static voteString_t validVoteStrings[] = {
	//	vote string				aliases										# args	valid gametypes							exec delay		short help
	{ "capturelimit",			"caps",				G_VoteCapturelimit,		1,		GTB_CTF | GTB_CTY,						qtrue,			"<num>" },
	{ "clientkick",			NULL,				G_VoteClientkick,		1,		GTB_ALL,								qfalse,			"<clientnum>" },
	{ "forcespec",			"forcespec",		G_VoteForceSpec,		1,		GTB_ALL,								qfalse,			"<clientnum>" },
	{ "fraglimit",			"frags",			G_VoteFraglimit,		1,		GTB_ALL & ~(GTB_CTF | GTB_CTY),	qtrue,			"<num>" },
	{ "g_doWarmup",			"dowarmup warmup",	G_VoteWarmup,			1,		GTB_ALL,								qtrue,			"<0-1>" },
	{ "g_gametype",			"gametype gt mode",	G_VoteGametype,			1,		GTB_ALL,								qtrue,			"<num or name>" },
	{ "kick",					NULL,				G_VoteKick,				1,		GTB_ALL,								qfalse,			"<client name>" },
	{ "map",					NULL,				G_VoteMap,				0,		GTB_ALL,								qtrue,			"<name>" },
	{ "map_restart",			"restart",			G_VoteMapRestart,		0,		GTB_ALL,								qtrue,			"<optional delay>" },
	{ "nextmap",				NULL,				G_VoteNextmap,			0,		GTB_ALL,								qtrue,			NULL },
	{ "sv_maxteamsize",		"teamsize",			G_VoteTeamSize,			1,		GTB_TEAM | GTB_CTY | GTB_CTF,		qtrue,			"<num>" },
	{ "timelimit",			"time",				G_VoteTimelimit,		1,		GTB_ALL,								qtrue,			"<num>" },
	{ "vstr",					"vstr",				G_VoteVSTR,				1,		GTB_ALL,								qtrue,			"<vstr name>" },
	{ "poll",					"poll",				G_VotePoll,				1,		GTB_ALL,								qfalse,			"<poll question>" },
	{ "pause",				"pause",			G_VotePause,			0,		GTB_ALL,								qfalse,			NULL },
	{ "score_restart",		"NULL",				G_VoteReset,			0,		GTB_ALL,								qfalse,			NULL },
};
static const int validVoteStringsSize = ARRAY_LEN(validVoteStrings);

VoteFloodProtect_t	voteFloodProtect[voteFloodProtectSize];//32 courses, 9 styles, 10 spots on highscore list
void TimeToString(int duration_ms, char *timeStr, size_t strSize, qboolean noMs);
char* strtok(char* str, const char* delimiters);
/*
==================
Cmd_CallVote_f
==================
*/
void Cmd_CallVote_f(gentity_t *ent) {
	int				i = 0, numArgs = 0;
	char			arg1[MAX_CVAR_VALUE_STRING] = { 0 };
	char			arg2[MAX_CVAR_VALUE_STRING] = { 0 };
	voteString_t	*vote = NULL;

	// not allowed to vote at all
	if (!g_allowVote.integer) {
		trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTE")));
		return;
	}

	if ((jp_tweakVote.integer & TV_MAPLOADTIMEOUT) && (level.startTime > (level.time - 1000 * 30))) { //Dont let a vote be called within 30sec of mapload ever
		trap_SendServerCommand(ent - g_entities, "print \"You are not allowed to callvote within 30 seconds of map load.\n\"");//print to wait X more minutes..seconds?
		return;
	} //fuck this stupid thing.. why does it work on 1 server but not the other..	

	if ((jp_fullAdminLevel.integer & JAPRO_ACCOUNTFLAG_A_CALLVOTE) || (jp_juniorAdminLevel.integer & JAPRO_ACCOUNTFLAG_A_CALLVOTE)) { //Admin only voting mode.. idk
		if (!G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_CALLVOTE, qfalse, qfalse, "callVote"))
			return;
	}

	// vote in progress
	if (level.voteTime) {
		trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "VOTEINPROGRESS")));
		return;
	}

	// can't vote as a spectator, except in (power)duel.. fuck this logic

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR || (ent->client->sess.sessionTeam == TEAM_FREE && g_gametype.integer >= GT_TEAM)) { //If we are in spec or racemode
		/*if (g_gametype.integer == GT_SAGA && jp_tweakVote.integer & TV_ALLOW_SIEGESPECVOTE) {
		}
		else */if (g_gametype.integer >= GT_TEAM && jp_tweakVote.integer & TV_ALLOW_CTFTFFASPECVOTE) {
		}
		else if (jp_tweakVote.integer & TV_ALLOW_SPECVOTE) {
		}
		else {
			trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOSPECVOTE")));
			return;
		}
	}

	/*
	if ( g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL && (ent->client->sess.sessionTeam == TEAM_SPECTATOR || (ent->client->sess.sessionTeam == TEAM_FREE && g_gametype.integer >= GT_TEAM))) {
	if (g_gametype.integer >= GT_TEAM || !g_tweakVote.integer) {
	trap_SendServerCommand( ent-g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "NOSPECVOTE" ) ) );
	return;
	}
	}
	*/

	if (jp_tweakVote.integer & TV_FLOODPROTECTBYIP) {
		char ourIP[NET_ADDRSTRMAXLEN] = { 0 };
		char *p = NULL;
		int j;

		Q_strncpyz(ourIP, ent->client->sess.IP, sizeof(ourIP));
		p = strchr(ourIP, ':');
		if (p)
			*p = 0;

		//trap_Print("Checking if client can vote: his ip is %s\n", ourIP);

		//Check if we are allowed to call vote
		if (jp_voteTimeout.integer) {
			const int time = trap_Milliseconds();
			for (j = 0; j<voteFloodProtectSize; j++) {
				//trap_Print("Searching slot: %i (%s, %i)\n", j, voteFloodProtect[j].ip, voteFloodProtect[j].voteTimeoutUntil);
				if (!Q_stricmp(voteFloodProtect[j].ip, ourIP)) {
					//trap_Print("Found clients IP in array!\n");
					const int voteTimeout = voteFloodProtect[j].failCount + 1 * 1000 * jp_voteTimeout.integer;

					if (voteFloodProtect[j].voteTimeoutUntil && (voteFloodProtect[j].voteTimeoutUntil > time)) { //compare this to something other than level.time ?
																												 //trap_Print("Client has just failed a vote, dont let them call this new one!\n");
						char timeStr[32];
						TimeToString((voteFloodProtect[j].voteTimeoutUntil - time), timeStr, sizeof(timeStr), qtrue);
						trap_SendServerCommand(ent - g_entities, va("print \"Please wait %s before calling a new vote.\n\"", timeStr));
						return;
					}
					break;
				}
				else if (!voteFloodProtect[j].ip[0]) {
					//trap_Print("Finished array search without finding clients IP! They have not failed a vote yet!\n");
					break;
				}
			}
		}

		//trap_Print("Client is allowed to call vote!\n");

		//We are allowed to call a vote if we get here
		Q_strncpyz(level.callVoteIP, ourIP, sizeof(level.callVoteIP));
	}

	// make sure it is a valid command to vote on
	numArgs = trap_Argc();
	trap_Argv(1, arg1, sizeof(arg1));
	if (numArgs > 1)
		Q_strncpyz(arg2, ConcatArgs(2), sizeof(arg2));

	// filter ; \n \r
	if (Q_strchrs(arg1, ";\r\n") || Q_strchrs(arg2, ";\r\n")) {
		trap_SendServerCommand(ent - g_entities, "print \"Invalid vote string.\n\"");
		return;
	}

	if ((jp_tweakVote.integer & TV_MAPCHANGELOCKOUT) && !Q_stricmp(arg1, "map") && (g_gametype.integer == GT_FFA || jp_raceMode.integer) && (level.startTime > (level.time - 1000 * 60 * 10))) { //Dont let a map vote be called within 10 mins of map load if we are in ffa
		char timeStr[32];
		TimeToString((1000 * 60 * 10 - (level.time - level.startTime)), timeStr, sizeof(timeStr), qtrue);
		trap_SendServerCommand(ent - g_entities, va("print \"The server just changed to this map, please wait %s before calling a map vote.\n\"", timeStr));
		return;
	}

	// check for invalid votes
	for (i = 0; i<validVoteStringsSize; i++) {
		if (!(g_allowVote.integer & (1 << i)))
			continue;

		if (!Q_stricmp(arg1, validVoteStrings[i].string))
			break;

		// see if they're using an alias, and set arg1 to the actual vote string
		if (validVoteStrings[i].aliases) {
			char tmp[MAX_TOKEN_CHARS] = { 0 }, *p = NULL;
			const char *delim = " ";
			Q_strncpyz(tmp, validVoteStrings[i].aliases, sizeof(tmp));
			p = strtok(tmp, delim);
			while (p != NULL) {
				if (!Q_stricmp(arg1, p)) {
					Q_strncpyz(arg1, validVoteStrings[i].string, sizeof(arg1));
					goto validVote;
				}
				p = strtok(NULL, delim);
			}
		}
	}
	// invalid vote string, abandon ship
	if (i == validVoteStringsSize) {
		char buf[1024] = { 0 };
		int toggle = 0;
		trap_SendServerCommand(ent - g_entities, "print \"Invalid vote string.\n\"");
		trap_SendServerCommand(ent - g_entities, "print \"Allowed vote strings are: \"");
		for (i = 0; i<validVoteStringsSize; i++) {
			if (!(g_allowVote.integer & (1 << i)))
				continue;

			toggle = !toggle;
			if (validVoteStrings[i].shortHelp) {
				Q_strcat(buf, sizeof(buf), va("^%c%s %s ",
					toggle ? COLOR_GREEN : COLOR_YELLOW,
					validVoteStrings[i].string,
					validVoteStrings[i].shortHelp));
			}
			else {
				Q_strcat(buf, sizeof(buf), va("^%c%s ",
					toggle ? COLOR_GREEN : COLOR_YELLOW,
					validVoteStrings[i].string));
			}
		}

		//FIXME: buffer and send in multiple messages in case of overflow
		trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", buf));
		return;
	}

validVote:
	vote = &validVoteStrings[i];
	if (!(vote->validGT & (1 << g_gametype.integer))) {
		trap_SendServerCommand(ent - g_entities, va("print \"%s is not applicable in this gametype.\n\"", arg1));
		return;
	}

	if (numArgs < vote->numArgs + 2) {
		trap_SendServerCommand(ent - g_entities, va("print \"%s requires more arguments: %s\n\"", arg1, vote->shortHelp));
		return;
	}

	level.votingGametype = qfalse;

	if (jp_tweakVote.integer & TV_MAPCHANGEVOTEDELAY) {
		if (!Q_stricmp(arg1, "map"))
			level.voteExecuteDelay = vote->voteDelay ? jp_voteDelay.integer : 0;
		else
			level.voteExecuteDelay = vote->voteDelay ? 3000 : 0;
	}
	else
		level.voteExecuteDelay = vote->voteDelay ? jp_voteDelay.integer : 0;

	// there is still a vote to be executed, execute it and store the new vote
	if (level.voteExecuteTime) { //bad idea
		if (jp_tweakVote.integer) { //wait what is this..
			trap_SendServerCommand(ent - g_entities, "print \"You are not allowed to call a new vote at this time.\n\"");//print to wait X more minutes..seconds?
			return;
		}
		else {
			level.voteExecuteTime = 0;
			trap_SendConsoleCommand(EXEC_APPEND, va("%s\n", level.voteString));
		}
	}

	// pass the args onto vote-specific handlers for parsing/filtering
	if (vote->func) {
		if (!vote->func(ent, numArgs, arg1, arg2))
			return;
	}
	// otherwise assume it's a command
	else {
		Com_sprintf(level.voteString, sizeof(level.voteString), "%s \"%s\"", arg1, arg2);
		Q_strncpyz(level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString));
		Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	}
	Q_strstrip(level.voteStringClean, "\"\n\r", NULL);

	trap_SendServerCommand(-1, va("print \"%s^7 %s (%s^7)\n\"", ent->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLCALLEDVOTE"), level.voteStringClean));
	G_LogPrintf("%s^7 %s (%s^7)\n", ent->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLCALLEDVOTE"), level.voteStringClean);

	// start the voting, the caller automatically votes yes
	level.voteTime = level.time;
	if (!Q_stricmp(arg1, "poll"))
		level.voteYes = 0;
	else
		level.voteYes = 1;
	level.voteNo = 0;

	for (i = 0; i<level.maxclients; i++) {
		level.clients[i].mGameFlags &= ~PSG_VOTED;
		level.clients[i].pers.vote = 0;
	}

	if (Q_stricmp(arg1, "poll")) {
		ent->client->mGameFlags |= PSG_VOTED;
		ent->client->pers.vote = 1;
	}

	trap_SetConfigstring(CS_VOTE_TIME, va("%i", level.voteTime));
	//trap_SetConfigstring( CS_VOTE_STRING,	level.voteDisplayString );	
	trap_SetConfigstring(CS_VOTE_STRING, va("%s", level.voteDisplayString));	 //dunno why this has to be done here..

	trap_SetConfigstring(CS_VOTE_YES, va("%i", level.voteYes));
	trap_SetConfigstring(CS_VOTE_NO, va("%i", level.voteNo));
}

/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f( gentity_t *ent ) {
	char		msg[64];

	if ( !level.voteTime ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOVOTEINPROG")) );
		return;
	}
	if ( ent->client->ps.eFlags & EF_VOTED ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "VOTEALREADY")) );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOVOTEASSPEC")) );
		return;
	}

	trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "PLVOTECAST")) );

	ent->client->ps.eFlags |= EF_VOTED;

	trap_Argv( 1, msg, sizeof( msg ) );

	if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
		level.voteYes++;
		trap_SetConfigstring( CS_VOTE_YES, va("%i", level.voteYes ) );
	} else {
		level.voteNo++;
		trap_SetConfigstring( CS_VOTE_NO, va("%i", level.voteNo ) );	
	}

	// a majority will be determined in CheckVote, which will also account
	// for players entering or leaving
}

/*
==================
Cmd_CallTeamVote_f
==================
*/
void Cmd_CallTeamVote_f( gentity_t *ent ) {
	int		i, cs_offset;
	team_t	team;
	char	arg1[MAX_STRING_TOKENS];
	char	arg2[MAX_STRING_TOKENS];

	team = ent->client->sess.sessionTeam;
	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !g_allowVote.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOVOTE")) );
		return;
	}

	if ( level.teamVoteTime[cs_offset] ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "TEAMVOTEALREADY")) );
		return;
	}
	if ( ent->client->pers.teamVoteCount >= MAX_VOTE_COUNT ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "MAXTEAMVOTES")) );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOSPECVOTE")) );
		return;
	}

	// make sure it is a valid command to vote on
	trap_Argv( 1, arg1, sizeof( arg1 ) );
	arg2[0] = '\0';
	for ( i = 2; i < trap_Argc(); i++ ) {
		if (i > 2)
			strcat(arg2, " ");
		trap_Argv( i, &arg2[strlen(arg2)], sizeof( arg2 ) - strlen(arg2) );
	}

	if( strchr( arg1, ';' ) || strchr( arg2, ';' ) ) {
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		return;
	}

	if ( !Q_stricmp( arg1, "leader" ) ) {
		char netname[MAX_NETNAME], leader[MAX_NETNAME];

		if ( !arg2[0] ) {
			i = ent->client->ps.clientNum;
		}
		else {
			// numeric values are just slot numbers
			for (i = 0; i < 3; i++) {
				if ( !arg2[i] || arg2[i] < '0' || arg2[i] > '9' )
					break;
			}
			if ( i >= 3 || !arg2[i]) {
				i = atoi( arg2 );
				if ( i < 0 || i >= level.maxclients ) {
					trap_SendServerCommand( ent-g_entities, va("print \"Bad client slot: %i\n\"", i) );
					return;
				}

				if ( !g_entities[i].inuse ) {
					trap_SendServerCommand( ent-g_entities, va("print \"Client %i is not active\n\"", i) );
					return;
				}
			}
			else {
				Q_strncpyz(leader, arg2, sizeof(leader));
				Q_CleanStr(leader, (qboolean)(jk2startversion == VERSION_1_02));
				for ( i = 0 ; i < level.maxclients ; i++ ) {
					if ( level.clients[i].pers.connected == CON_DISCONNECTED )
						continue;
					if (level.clients[i].sess.sessionTeam != team)
						continue;
					Q_strncpyz(netname, level.clients[i].pers.netname, sizeof(netname));
					Q_CleanStr(netname, (qboolean)(jk2startversion == VERSION_1_02));
					if ( !Q_stricmp(netname, leader) ) {
						break;
					}
				}
				if ( i >= level.maxclients ) {
					trap_SendServerCommand( ent-g_entities, va("print \"%s is not a valid player on your team.\n\"", arg2) );
					return;
				}
			}
		}
		Com_sprintf(arg2, sizeof(arg2), "%d", i);
	} else {
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		trap_SendServerCommand( ent-g_entities, "print \"Team vote commands are: leader <player>.\n\"" );
		return;
	}

	Com_sprintf( level.teamVoteString[cs_offset], sizeof( level.teamVoteString[cs_offset] ), "%s %s", arg1, arg2 );

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_DISCONNECTED )
			continue;
		if (level.clients[i].sess.sessionTeam == team)
			trap_SendServerCommand( i, va("print \"%s" S_COLOR_WHITE " called a team vote.\n\"", ent->client->pers.netname ) );
	}

	// start the voting, the caller autoamtically votes yes
	level.teamVoteTime[cs_offset] = level.time;
	level.teamVoteYes[cs_offset] = 1;
	level.teamVoteNo[cs_offset] = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam == team)
			level.clients[i].ps.eFlags &= ~EF_TEAMVOTED;
	}
	ent->client->ps.eFlags |= EF_TEAMVOTED;

	trap_SetConfigstring( CS_TEAMVOTE_TIME + cs_offset, va("%i", level.teamVoteTime[cs_offset] ) );
	trap_SetConfigstring( CS_TEAMVOTE_STRING + cs_offset, level.teamVoteString[cs_offset] );
	trap_SetConfigstring( CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamVoteYes[cs_offset] ) );
	trap_SetConfigstring( CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamVoteNo[cs_offset] ) );
}

/*
==================
Cmd_TeamVote_f
==================
*/
void Cmd_TeamVote_f( gentity_t *ent ) {
	int			team, cs_offset;
	char		msg[64];

	team = ent->client->sess.sessionTeam;
	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !level.teamVoteTime[cs_offset] ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOTEAMVOTEINPROG")) );
		return;
	}
	if ( ent->client->ps.eFlags & EF_TEAMVOTED ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "TEAMVOTEALREADYCAST")) );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOVOTEASSPEC")) );
		return;
	}

	trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "PLTEAMVOTECAST")) );

	ent->client->ps.eFlags |= EF_TEAMVOTED;

	trap_Argv( 1, msg, sizeof( msg ) );

	if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
		level.teamVoteYes[cs_offset]++;
		trap_SetConfigstring( CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamVoteYes[cs_offset] ) );
	} else {
		level.teamVoteNo[cs_offset]++;
		trap_SetConfigstring( CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamVoteNo[cs_offset] ) );	
	}

	// a majority will be determined in TeamCheckVote, which will also account
	// for players entering or leaving
}


/*
=================
Cmd_SetViewpos_f
=================
*/
void Cmd_SetViewpos_f( gentity_t *ent ) {
	vec3_t		origin, angles;
	char		buffer[MAX_TOKEN_CHARS];
	int			i;

	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOCHEATS")));
		return;
	}
	if ( trap_Argc() != 5 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"usage: setviewpos x y z yaw\n\""));
		return;
	}

	VectorClear( angles );
	for ( i = 0 ; i < 3 ; i++ ) {
		trap_Argv( i + 1, buffer, sizeof( buffer ) );
		origin[i] = atof( buffer );
	}

	trap_Argv( 4, buffer, sizeof( buffer ) );
	angles[YAW] = atof( buffer );

	TeleportPlayer( ent, origin, angles );
}



/*
=================
Cmd_Stats_f
=================
*/
void Cmd_Stats_f( gentity_t *ent ) {
/*
	int max, n, i;

	max = trap_AAS_PointReachabilityAreaIndex( NULL );

	n = 0;
	for ( i = 0; i < max; i++ ) {
		if ( ent->client->areabits[i >> 3] & (1 << (i & 7)) )
			n++;
	}

	//trap_SendServerCommand( ent-g_entities, va("print \"visited %d of %d areas\n\"", n, max));
	trap_SendServerCommand( ent-g_entities, va("print \"%d%% level coverage\n\"", n * 100 / max));
*/
}

int G_ItemUsable(playerState_t *ps, int forcedUse)
{
	vec3_t fwd, fwdorg, dest, pos;
	vec3_t yawonly;
	vec3_t mins, maxs;
	vec3_t trtest;
	trace_t tr;

	if (ps->usingATST)
	{
		return 0;
	}
	
	if (ps->pm_flags & PMF_USE_ITEM_HELD)
	{ //force to let go first
		return 0;
	}

	if (!forcedUse)
	{
		forcedUse = bg_itemlist[ps->stats[STAT_HOLDABLE_ITEM]].giTag;
	}

	switch (forcedUse)
	{
	case HI_MEDPAC:
		if (ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH])
		{
			return 0;
		}

		if (ps->stats[STAT_HEALTH] <= 0)
		{
			return 0;
		}

		return 1;
	case HI_SEEKER:
		if (ps->eFlags & EF_SEEKERDRONE)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SEEKER_ALREADYDEPLOYED);
			return 0;
		}

		return 1;
	case HI_SENTRY_GUN:
		if (ps->fd.sentryDeployed)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SENTRY_ALREADYPLACED);
			return 0;
		}

		yawonly[ROLL] = 0;
		yawonly[PITCH] = 0;
		yawonly[YAW] = ps->viewangles[YAW];

		VectorSet( mins, -8, -8, 0 );
		VectorSet( maxs, 8, 8, 24 );

		AngleVectors(yawonly, fwd, NULL, NULL);

		fwdorg[0] = ps->origin[0] + fwd[0]*64;
		fwdorg[1] = ps->origin[1] + fwd[1]*64;
		fwdorg[2] = ps->origin[2] + fwd[2]*64;

		trtest[0] = fwdorg[0] + fwd[0]*16;
		trtest[1] = fwdorg[1] + fwd[1]*16;
		trtest[2] = fwdorg[2] + fwd[2]*16;

		trap_Trace(&tr, ps->origin, mins, maxs, trtest, ps->clientNum, MASK_PLAYERSOLID);

		if ((tr.fraction != 1 && tr.entityNum != ps->clientNum) || tr.startsolid || tr.allsolid)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SENTRY_NOROOM);
			return 0;
		}

		return 1;
	case HI_SHIELD:
		mins[0] = -8;
		mins[1] = -8;
		mins[2] = 0;

		maxs[0] = 8;
		maxs[1] = 8;
		maxs[2] = 8;

		AngleVectors (ps->viewangles, fwd, NULL, NULL);
		fwd[2] = 0;
		VectorMA(ps->origin, 64, fwd, dest);
		trap_Trace(&tr, ps->origin, mins, maxs, dest, ps->clientNum, MASK_SHOT );
		if (tr.fraction > 0.9 && !tr.startsolid && !tr.allsolid)
		{
			VectorCopy(tr.endpos, pos);
			VectorSet( dest, pos[0], pos[1], pos[2] - 4096 );
			trap_Trace( &tr, pos, mins, maxs, dest, ps->clientNum, MASK_SOLID );
			if ( !tr.startsolid && !tr.allsolid )
			{
				return 1;
			}
		}
		G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SHIELD_NOROOM);
		return 0;
	default:
		return 1;
	}
}

extern int saberOffSound;
extern int saberOnSound;

void Cmd_ToggleSaber_f(gentity_t *ent)
{
	if (!saberOffSound || !saberOnSound)
	{
		saberOffSound = G_SoundIndex("sound/weapons/saber/saberoffquick.wav");
		saberOnSound = G_SoundIndex("sound/weapons/saber/saberon.wav");
	}

	if (ent->client->ps.saberInFlight)
	{
		return;
	}

	if (ent->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return;
	}

	if (ent->client->ps.weapon != WP_SABER)
	{
		return;
	}

//	if (ent->client->ps.duelInProgress && !ent->client->ps.saberHolstered)
//	{
//		return;
//	}

	if (ent->client->ps.duelTime >= level.time)
	{
		return;
	}

	if (ent->client->ps.saberLockTime >= level.time)
	{
		return;
	}

	if (ent->client && ent->client->ps.weaponTime < 1)
	{
		if (ent->client->ps.saberHolstered)
		{
			ent->client->ps.saberHolstered = qfalse;
			G_Sound(ent, CHAN_AUTO, saberOnSound);
		}
		else
		{
			ent->client->ps.saberHolstered = qtrue;
			G_Sound(ent, CHAN_AUTO, saberOffSound);

			//prevent anything from being done for 400ms after holster
			ent->client->ps.weaponTime = 400;
		}
	}
}

void Cmd_SaberAttackCycle_f(gentity_t *ent)
{
	int selectLevel = 0;

	if ( !ent || !ent->client )
	{
		return;
	}
	/*
	if (ent->client->ps.weaponTime > 0)
	{ //no switching attack level when busy
		return;
	}
	*/

	if (ent->client->saberCycleQueue)
	{ //resume off of the queue if we haven't gotten a chance to update it yet
		selectLevel = ent->client->saberCycleQueue;
	}
	else
	{
		selectLevel = ent->client->ps.fd.saberAnimLevel;
	}

	selectLevel++;
	if ( selectLevel > ent->client->ps.fd.forcePowerLevel[FP_SABERATTACK] )
	{
		selectLevel = FORCE_LEVEL_1;
	}
/*
#ifndef FINAL_BUILD
	switch ( selectLevel )
	{
	case FORCE_LEVEL_1:
		trap_SendServerCommand( ent-g_entities, va("print \"Lightsaber Combat Style: %sfast\n\"", S_COLOR_BLUE) );
		break;
	case FORCE_LEVEL_2:
		trap_SendServerCommand( ent-g_entities, va("print \"Lightsaber Combat Style: %smedium\n\"", S_COLOR_YELLOW) );
		break;
	case FORCE_LEVEL_3:
		trap_SendServerCommand( ent-g_entities, va("print \"Lightsaber Combat Style: %sstrong\n\"", S_COLOR_RED) );
		break;
	}
#endif
*/
	if (ent->client->ps.weaponTime <= 0)
	{ //not busy, set it now
		ent->client->ps.fd.saberAnimLevel = selectLevel;
	}
	else
	{ //can't set it now or we might cause unexpected chaining, so queue it
		ent->client->saberCycleQueue = selectLevel;
	}
}

qboolean G_OtherPlayersDueling(void)
{
	int i = 0;
	gentity_t *ent;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->inuse && ent->client && ent->client->ps.duelInProgress)
		{
			return qtrue;
		}
		i++;
	}

	return qfalse;
}

void Cmd_EngageDuel_f(gentity_t *ent)
{
	trace_t tr;
	vec3_t forward, fwdOrg;

	if (!g_privateDuel.integer)
	{
		return;
	}

	if (g_gametype.integer == GT_TOURNAMENT)
	{ //rather pointless in this mode..
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NODUEL_GAMETYPE")) );
		return;
	}

	if (g_gametype.integer >= GT_TEAM)
	{ //no private dueling in team modes
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NODUEL_GAMETYPE")) );
		return;
	}

	if (ent->client->ps.duelTime >= level.time)
	{
		return;
	}

	if (ent->client->ps.weapon != WP_SABER)
	{
		return;
	}

	/*
	if (!ent->client->ps.saberHolstered)
	{ //must have saber holstered at the start of the duel
		return;
	}
	*/
	//NOTE: No longer doing this..

	if (ent->client->ps.saberInFlight)
	{
		return;
	}

	if (ent->client->ps.duelInProgress)
	{
		return;
	}

	//New: Don't let a player duel if he just did and hasn't waited 10 seconds yet (note: If someone challenges him, his duel timer will reset so he can accept)
	//Fuck that ^^^^^^
	/*if (ent->client->ps.fd.privateDuelTime > level.time)
	{
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "CANTDUEL_JUSTDID")) );
		return;
	}

	if (G_OtherPlayersDueling())
	{
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "CANTDUEL_BUSY")) );
		return;
	}*/

	AngleVectors( ent->client->ps.viewangles, forward, NULL, NULL );

	fwdOrg[0] = ent->client->ps.origin[0] + forward[0]*256;
	fwdOrg[1] = ent->client->ps.origin[1] + forward[1]*256;
	fwdOrg[2] = (ent->client->ps.origin[2]+ent->client->ps.viewheight) + forward[2]*256;

	trap_Trace(&tr, ent->client->ps.origin, NULL, NULL, fwdOrg, ent->s.number, MASK_PLAYERSOLID);

	if (tr.fraction != 1 && tr.entityNum < MAX_CLIENTS)
	{
		gentity_t *challenged = &g_entities[tr.entityNum];

		if (!challenged || !challenged->client || !challenged->inuse ||
			challenged->health < 1 || challenged->client->ps.stats[STAT_HEALTH] < 1 ||
			challenged->client->ps.weapon != WP_SABER || challenged->client->ps.duelInProgress ||
			challenged->client->ps.saberInFlight)
		{
			return;
		}

		if (g_gametype.integer >= GT_TEAM && OnSameTeam(ent, challenged))
		{
			return;
		}

		if (challenged->client->ps.duelIndex == ent->s.number && challenged->client->ps.duelTime >= level.time)
		{
			//trap_SendServerCommand( /*challenged-g_entities*/-1, va("print \"%s" S_COLOR_WHITE " %s %s!\n\"", challenged->client->pers.netname, G_GetStripEdString("SVINGAME", "PLDUELACCEPT"), ent->client->pers.netname) );
			trap_SendServerCommand(-1, va("print \"%s^7 %s %s^7!\n\"", challenged->client->pers.netname, G_GetStripEdString("MP_SVGAME", "PLDUELACCEPT"), ent->client->pers.netname));

			ent->client->ps.duelInProgress = qtrue;
			challenged->client->ps.duelInProgress = qtrue;

			ent->client->ps.duelTime = level.time + 2000;
			challenged->client->ps.duelTime = level.time + 2000;

			G_AddEvent(ent, EV_PRIVATE_DUEL, 1);
			G_AddEvent(challenged, EV_PRIVATE_DUEL, 1);

			//Holster their sabers now, until the duel starts (then they'll get auto-turned on to look cool)

			if (!ent->client->ps.saberHolstered)
			{
				G_Sound(ent, CHAN_AUTO, saberOffSound);
				ent->client->ps.weaponTime = 400;
				ent->client->ps.saberHolstered = qtrue;
			}
			if (!challenged->client->ps.saberHolstered)
			{
				G_Sound(challenged, CHAN_AUTO, saberOffSound);
				challenged->client->ps.weaponTime = 400;
				challenged->client->ps.saberHolstered = qtrue;
			}
			if (jp_duelStartHealth.integer)
			{
				ent->health = ent->client->ps.stats[STAT_HEALTH] = jp_duelStartHealth.integer;
				challenged->health = challenged->client->ps.stats[STAT_HEALTH] =jp_duelStartHealth.integer;
			}
			if (jp_duelStartArmor.integer)
			{
				ent->client->ps.stats[STAT_ARMOR] = jp_duelStartArmor.integer;
				challenged->client->ps.stats[STAT_ARMOR] = jp_duelStartArmor.integer;
			}
		}
		else
		{
			//Print the message that a player has been challenged in private, only announce the actual duel initiation in private
			trap_SendServerCommand( challenged-g_entities, va("cp \"%s" S_COLOR_WHITE " %s\n\"", ent->client->pers.netname, G_GetStripEdString("SVINGAME", "PLDUELCHALLENGE")) );
			trap_SendServerCommand( ent-g_entities, va("cp \"%s %s\n\"", G_GetStripEdString("SVINGAME", "PLDUELCHALLENGED"), challenged->client->pers.netname) );
		}

		challenged->client->ps.fd.privateDuelTime = 0; //reset the timer in case this player just got out of a duel. He should still be able to accept the challenge.

		ent->client->ps.forceHandExtend = HANDEXTEND_DUELCHALLENGE;
		ent->client->ps.forceHandExtendTime = level.time + 1000;

		ent->client->ps.duelIndex = challenged->s.number;
		ent->client->ps.duelTime = level.time + 5000;
	}
}

static void Cmd_Hide_f(gentity_t *ent)
{
	if (!ent->client)
		return;

	if (trap_Argc() != 1) {
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /hide\n\"");
		return;
	}

	if (!G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_NOFOLLOW, qfalse, jp_allowNoFollow.integer, "hide")) //idk
		return;

	if (ent->client->pers.stats.startTime || ent->client->pers.stats.startTimeFlag) {
		trap_SendServerCommand(ent - g_entities, "print \"Hide status updated: timer reset.\n\"");
		ResetPlayerTimers(ent, qtrue);
	}

	ent->client->pers.noFollow = (qboolean)!ent->client->pers.noFollow;

	if (ent->client->sess.raceMode && jp_allowNoFollow.integer > 1) { // > 1 makes them invis ingame as well if racemode
		if (ent->client->pers.noFollow) {
			ent->r.svFlags |= SVF_SINGLECLIENT;
			ent->r.singleClient = ent->s.number;
		}
		else
			ent->r.svFlags &= ~SVF_SINGLECLIENT;
	}

	if (ent->client->pers.noFollow)
		trap_SendServerCommand(ent - g_entities, "print \"You can not be spectated now.\n\"");
	else
		trap_SendServerCommand(ent - g_entities, "print \"You can be spectated now.\n\"");
}

/*
=================
Cmd_Amlogin_f
=================
*/
void Cmd_Amlogin_f(gentity_t *ent)
{
	char   pass[MAX_STRING_CHARS];

	trap_Argv(1, pass, sizeof(pass)); //Password

	if (!ent->client)
		return;

	if (trap_Argc() == 1)
	{
		trap_SendServerCommand(ent - g_entities, "print \"Usage: amLogin <password>\n\"");
		return;
	}
	if (trap_Argc() == 2)
	{
		qboolean added = qfalse; //Check if they would gain anything from logging in, return if not.
		int i;

		if (!Q_stricmp(pass, "")) {
			trap_SendServerCommand(ent - g_entities, "print \"Usage: amLogin <password>\n\"");
		}
		else if (!Q_stricmp(pass, jp_fullAdminPass.string)) {
			for (i = 0; i <= JAPRO_MAX_ADMIN_BITS; i++) {//Loop this 0-22 is admin flags.
				if (jp_fullAdminLevel.integer & (1 << i)) {
					if (!(ent->client->sess.accountFlags & (1 << i))) { //They don't already have it
						added = qtrue;
						ent->client->sess.accountFlags |= (1 << i);
					}
				}
			}

			if (added) {
				if (Q_stricmp(jp_fullAdminMsg.string, "")) //Ok, so just set this to " " if you want it to print the normal login msg, or set it to "" to skip.  or "with junior admin" for more info.. etc
					trap_SendServerCommand(-1, va("print \"%s^7 has logged in %s\n\"", ent->client->pers.netname, jp_fullAdminMsg.string));
				else
					trap_SendServerCommand(ent - g_entities, "print \"^2You are now logged in with full admin privileges.\n\"");
			}
			else {
				trap_SendServerCommand(ent - g_entities, "print \"You are already logged in. Type in /amLogout to remove admin status.\n\"");
			}
		}
		else if (!Q_stricmp(pass, jp_juniorAdminPass.string)) {
			for (i = 0; i <= JAPRO_MAX_ADMIN_BITS; i++) {//Loop this 0-22 is admin flags.
				if (jp_juniorAdminLevel.integer & (1 << i)) {
					if (!(ent->client->sess.accountFlags & (1 << i))) { //They don't already have it
						added = qtrue;
						ent->client->sess.accountFlags |= (1 << i);
					}
				}
			}

			if (added) {
				if (Q_stricmp(jp_juniorAdminMsg.string, ""))
					trap_SendServerCommand(-1, va("print \"%s^7 has logged in %s\n\"", ent->client->pers.netname, jp_juniorAdminMsg.string));
				else
					trap_SendServerCommand(ent - g_entities, "print \"^2You are now logged in with junior admin privileges.\n\"");
			}
			else {
				trap_SendServerCommand(ent - g_entities, "print \"You are already logged in. Type in /amLogout to remove admin status.\n\"");
			}
		}
		else {
			trap_SendServerCommand(ent - g_entities, "print \"^3Failed to log in: Incorrect password!\n\"");
		}
	}
}

/*
=================
Cmd_Amlogout_f
=================
*/
void Cmd_Amlogout_f(gentity_t *ent)
{
	int i;
	qboolean found = qfalse;

	if (!ent->client)
		return;

	for (i = 0; i <= JAPRO_MAX_ADMIN_BITS; i++) {//Loop this 0-22 is admin flags.
		if (ent->client->sess.accountFlags & (1 << i)) {
			ent->client->sess.accountFlags &= ~(1 << i);
			found = qtrue;
		}
	}

	if (found)
		trap_SendServerCommand(ent - g_entities, "print \"You are no longer an admin.\n\"");
	else
		trap_SendServerCommand(ent - g_entities, "print \"You are not logged in as an admin!\n\"");

}

//[JAPRO - Serverside - All - Amrename - Start]
void Cmd_Amrename_f(gentity_t *ent)
{
	int		clientid = -1;
	char	arg[MAX_STRING_CHARS];
	char	userinfo[MAX_INFO_STRING];

	if (!ent->client)
		return;

	if (!G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_RENAME, qfalse, qfalse, "amRename"))
		return;

	if (trap_Argc() != 3) {
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /amRename <client> <newname>.\n\"");
		return;
	}
	trap_Argv(1, arg, sizeof(arg));

	clientid = JP_ClientNumberFromString(ent, arg);

	if (clientid == -1 || clientid == -2)
		return;

	trap_Argv(2, arg, sizeof(arg));

	trap_GetUserinfo(clientid, userinfo, sizeof(userinfo));
	Info_SetValueForKey(userinfo, "name", arg);
	trap_SetUserinfo(clientid, userinfo);
	ClientUserinfoChanged(clientid);
	level.clients[clientid].pers.netnameTime = level.time + 5000;
}


void Cmd_Amtelemark_f(gentity_t *ent)
{
	if (!ent->client)
		return;

	if (!G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_TELEMARK, qtrue, qtrue, "amTelemark"))
		return;

	VectorCopy(ent->client->ps.origin, ent->client->pers.telemarkOrigin);
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR && (ent->client->ps.pm_flags & PMF_FOLLOW))
		ent->client->pers.telemarkOrigin[2] += 58;
	ent->client->pers.telemarkAngle = ent->client->ps.viewangles[YAW];
	ent->client->pers.telemarkPitchAngle = ent->client->ps.viewangles[PITCH];
	trap_SendServerCommand(ent - g_entities, va("print \"Teleport Marker: ^3<%i, %i, %i> %i, %i\n\"",
		(int)ent->client->pers.telemarkOrigin[0], (int)ent->client->pers.telemarkOrigin[1], (int)ent->client->pers.telemarkOrigin[2], (int)ent->client->pers.telemarkAngle, (int)ent->client->pers.telemarkPitchAngle));
}

void Cmd_Amtele_f(gentity_t *ent)
{
	gentity_t	*teleporter;// = NULL;
	char client1[MAX_NETNAME], client2[MAX_NETNAME];
	char x[32], y[32], z[32], yaw[32];
	int clientid1 = -1, clientid2 = -1;
	vec3_t	angles = { 0, 0, 0 }, origin;
	qboolean droptofloor = qfalse, race = qfalse;
	int allowed;

	if (!ent->client)
		return;

	allowed = G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_ADMINTELE, qtrue, qtrue, "amTele");

	if (allowed == 3) { //admin allowed

		if (ent->client->sess.raceMode) {
			droptofloor = qtrue;
			race = qtrue;
		}

		if (trap_Argc() > 6)
		{
			trap_SendServerCommand(ent - g_entities, "print \"Usage: /amTele or /amTele <client> or /amTele <client> <client> or /amTele <X> <Y> <Z> <YAW> or /amTele <player> <X> <Y> <Z> <YAW>.\n\"");
			return;
		}

		if (trap_Argc() == 1)//Amtele to telemark
		{
			if (ent->client->pers.telemarkOrigin[0] != 0 || ent->client->pers.telemarkOrigin[1] != 0 || ent->client->pers.telemarkOrigin[2] != 0 || ent->client->pers.telemarkAngle != 0)
			{
				angles[YAW] = ent->client->pers.telemarkAngle;
				angles[PITCH] = ent->client->pers.telemarkPitchAngle;
				AmTeleportPlayer(ent, ent->client->pers.telemarkOrigin, angles, droptofloor, race, qfalse);
			}
			else
				trap_SendServerCommand(ent - g_entities, "print \"No telemark set!\n\"");
			return;
		}

		if (trap_Argc() == 2)//Amtele to player
		{
			trap_Argv(1, client1, sizeof(client1));
			clientid1 = JP_ClientNumberFromString(ent, client1);

			if (clientid1 == -1 || clientid1 == -2)
				return;

			if (g_entities[clientid1].client->pers.noFollow || g_entities[clientid1].client->sess.sessionTeam == TEAM_SPECTATOR) {
				if (!G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_SEEHIDDEN, qfalse, qfalse, NULL))
					return;
			}

			origin[0] = g_entities[clientid1].client->ps.origin[0];
			origin[1] = g_entities[clientid1].client->ps.origin[1];
			origin[2] = g_entities[clientid1].client->ps.origin[2] + 96;
			AmTeleportPlayer(ent, origin, angles, droptofloor, race, qfalse);
			return;
		}

		if (trap_Argc() == 3)//Amtele player to player
		{
			trap_Argv(1, client1, sizeof(client1));
			trap_Argv(2, client2, sizeof(client2));
			clientid1 = JP_ClientNumberFromString(ent, client1);
			clientid2 = JP_ClientNumberFromString(ent, client2);

			if (clientid1 == -1 || clientid1 == -2 || clientid2 == -1 || clientid2 == -2)
				return;

			if (g_entities[clientid2].client->pers.noFollow || g_entities[clientid2].client->sess.sessionTeam == TEAM_SPECTATOR) {
				if (!G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_SEEHIDDEN, qfalse, qfalse, NULL))
					return;
			}

			if (!G_AdminUsableOn(ent->client, g_entities[clientid1].client, JAPRO_ACCOUNTFLAG_A_ADMINTELE)) {
				if (g_entities[clientid1].client->ps.clientNum != ent->client->ps.clientNum)
					return;
				else
					trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command on this player (amTele).\n\"");
			}

			teleporter = &g_entities[clientid1];

			origin[0] = g_entities[clientid2].client->ps.origin[0];
			origin[1] = g_entities[clientid2].client->ps.origin[1];
			origin[2] = g_entities[clientid2].client->ps.origin[2] + 96;

			AmTeleportPlayer(teleporter, origin, angles, droptofloor, qfalse, qfalse);
			return;
}

		if (trap_Argc() == 4)
		{
			trap_Argv(1, x, sizeof(x));
			trap_Argv(2, y, sizeof(y));
			trap_Argv(3, z, sizeof(z));

			origin[0] = atoi(x);
			origin[1] = atoi(y);
			origin[2] = atoi(z);

			/*if (trap_Argc() == 5)
			{
			trap_Argv(4, yaw, sizeof(yaw));
			angles[YAW] = atoi(yaw);
			}*/

			AmTeleportPlayer(ent, origin, angles, droptofloor, race, qfalse);
			return;
		}

		if (trap_Argc() == 5)//Amtele to angles + origin, OR Amtele player to origin
		{
			trap_Argv(1, client1, sizeof(client1));
			clientid1 = JP_ClientNumberFromString(ent, client1);

			if (clientid1 == -1 || clientid1 == -2)//Amtele to origin + angles
			{
				trap_Argv(1, x, sizeof(x));
				trap_Argv(2, y, sizeof(y));
				trap_Argv(3, z, sizeof(z));

				origin[0] = atoi(x);
				origin[1] = atoi(y);
				origin[2] = atoi(z);

				trap_Argv(4, yaw, sizeof(yaw));
				angles[YAW] = atoi(yaw);

				AmTeleportPlayer(ent, origin, angles, droptofloor, race, qfalse);
			}

			else//Amtele other player to origin
			{
				if (!G_AdminUsableOn(ent->client, g_entities[clientid1].client, JAPRO_ACCOUNTFLAG_A_ADMINTELE)) {
					if (g_entities[clientid1].client->ps.clientNum != ent->client->ps.clientNum)
						return;
					else
						trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command on this player (amTele).\n\"");
				}

				teleporter = &g_entities[clientid1];

				trap_Argv(2, x, sizeof(x));
				trap_Argv(3, y, sizeof(y));
				trap_Argv(4, z, sizeof(z));

				origin[0] = atoi(x);
				origin[1] = atoi(y);
				origin[2] = atoi(z);

				AmTeleportPlayer(teleporter, origin, angles, droptofloor, qfalse, qfalse);
			}
			return;

		}

		if (trap_Argc() == 6)//Amtele player to angles + origin
		{
			trap_Argv(1, client1, sizeof(client1));
			clientid1 = JP_ClientNumberFromString(ent, client1);

			if (clientid1 == -1 || clientid1 == -2)
				return;

			if (!G_AdminUsableOn(ent->client, g_entities[clientid1].client, JAPRO_ACCOUNTFLAG_A_ADMINTELE)) {
				if (g_entities[clientid1].client->ps.clientNum != ent->client->ps.clientNum)
					return;
				else
					trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command on this player (amTele).\n\"");
			}

			teleporter = &g_entities[clientid1];

			trap_Argv(2, x, sizeof(x));
			trap_Argv(3, y, sizeof(y));
			trap_Argv(4, z, sizeof(z));

			origin[0] = atoi(x);
			origin[1] = atoi(y);
			origin[2] = atoi(z);

			trap_Argv(5, yaw, sizeof(yaw));
			angles[YAW] = atoi(yaw);

			AmTeleportPlayer(teleporter, origin, angles, droptofloor, qfalse, qfalse);
			return;
		}

	}
	else if (allowed) { //Cheat or racemode
		//Cmd_RaceTele_f(ent);
		return;
	}
}

//[JAPRO - Serverside - All - Amlockteam Function - Start]
/*
=================
Cmd_Amlockteam_f
=================
*/
void Cmd_Amlockteam_f(gentity_t *ent)
{
	char teamname[MAX_TEAMNAME];

	if (!ent->client)
		return;

	if (!G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_LOCKTEAM, qfalse, qfalse, "amLockTeam"))
		return;

	if (g_gametype.integer >= GT_TEAM || g_gametype.integer == GT_FFA)
	{
		if (trap_Argc() != 2)
		{
			trap_SendServerCommand(ent - g_entities, "print \"Usage: /amLockTeam <team>\n\"");
			return;
		}

		trap_Argv(1, teamname, sizeof(teamname));

		if (!Q_stricmp(teamname, "red") || !Q_stricmp(teamname, "r"))
		{
			if (level.isLockedred == qfalse)
			{
				level.isLockedred = qtrue;
				trap_SendServerCommand(-1, "print \"The Red team is now locked.\n\"");
			}
			else
			{
				level.isLockedred = qfalse;
				trap_SendServerCommand(-1, "print \"The Red team is now unlocked.\n\"");
			}
		}
		else if (!Q_stricmp(teamname, "blue") || !Q_stricmp(teamname, "b"))
		{
			if (level.isLockedblue == qfalse)
			{
				level.isLockedblue = qtrue;
				trap_SendServerCommand(-1, "print \"The Blue team is now locked.\n\"");
			}
			else
			{
				level.isLockedblue = qfalse;
				trap_SendServerCommand(-1, "print \"The Blue team is now unlocked.\n\"");
			}
		}
		else if (!Q_stricmp(teamname, "s") || !Q_stricmp(teamname, "spectator") || !Q_stricmp(teamname, "spec") || !Q_stricmp(teamname, "spectate"))
		{
			if (level.isLockedspec == qfalse)
			{
				level.isLockedspec = qtrue;
				trap_SendServerCommand(-1, "print \"The Spectator team is now locked.\n\"");
			}
			else
			{
				level.isLockedspec = qfalse;
				trap_SendServerCommand(-1, "print \"The spectator team is now unlocked.\n\"");
			}
		}
		else if (!Q_stricmp(teamname, "f") || !Q_stricmp(teamname, "free") || !Q_stricmp(teamname, "join") || !Q_stricmp(teamname, "enter") || !Q_stricmp(teamname, "j"))
		{
			if (level.isLockedfree == qfalse)
			{
				level.isLockedfree = qtrue;
				trap_SendServerCommand(-1, "print \"The Free team is now locked.\n\"");
			}
			else
			{
				level.isLockedfree = qfalse;
				trap_SendServerCommand(-1, "print \"The Free team is now unlocked.\n\"");
			}
		}
		else
		{
			trap_SendServerCommand(ent - g_entities, "print \"Usage: /amLockTeam <team>\n\"");
			return;
		}
	}
	else
	{
		trap_SendServerCommand(ent - g_entities, "print \"You can not use this command in this gametype (amLockTeam).\n\"");
		return;
	}
}
//[JAPRO - Serverside - All - Amlockteam Function - End]

//[JAPRO - Serverside - All - Amforceteam Function - Start]
/*
=================
Cmd_Amforceteam_f

=================
*/
void Cmd_Amforceteam_f(gentity_t *ent)
{
	char arg[MAX_NETNAME];
	char teamname[MAX_TEAMNAME];
	int  clientid = 0;//stfu compiler

	if (!G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_FORCETEAM, qfalse, qfalse, "amForceTeam"))
		return;

	if (trap_Argc() != 3)
	{
		trap_SendServerCommand(ent - g_entities, "print \"Usage: amForceTeam <client> <team>\n\"");
		return;
	}

	if (g_gametype.integer >= GT_TEAM || g_gametype.integer == GT_FFA)
	{
		qboolean everyone = qfalse;
		gclient_t *client;
		int i;

		trap_Argv(1, arg, sizeof(arg));

		if (!Q_stricmp(arg, "-1"))
			everyone = qtrue;

		if (!everyone)
		{
			clientid = JP_ClientNumberFromString(ent, arg);

			if (clientid == -1 || clientid == -2)//No clients or multiple clients are a match
			{
				return;
			}

			if (!G_AdminUsableOn(ent->client, g_entities[clientid].client, JAPRO_ACCOUNTFLAG_A_FORCETEAM)) {
				if (g_entities[clientid].client->ps.clientNum != ent->client->ps.clientNum)
					return;
				else
					trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command on this player (amForceTeam).\n\"");
			}
		}

		trap_Argv(2, teamname, sizeof(teamname));

		if ((!Q_stricmp(teamname, "red") || !Q_stricmp(teamname, "r")) && g_gametype.integer >= GT_TEAM)
		{
			if (everyone)
			{
				for (i = 0, client = level.clients; i < level.maxclients; ++i, ++client)
				{
					if (client->pers.connected != CON_CONNECTED)//client->sess.sessionTeam
						continue;
					if (client->sess.sessionTeam != TEAM_RED)
						SetTeam(&g_entities[i], "red", qtrue);
				}
			}
			else
			{
				if (g_entities[clientid].client->sess.sessionTeam != TEAM_RED) {
					SetTeam(&g_entities[clientid], "red", qtrue);
					trap_SendServerCommand(-1, va("print \"%s ^7has been forced to the ^1Red ^7team.\n\"", g_entities[clientid].client->pers.netname));
				}
			}
		}
		else if ((!Q_stricmp(teamname, "blue") || !Q_stricmp(teamname, "b")) && g_gametype.integer >= GT_TEAM)
		{
			if (everyone)
			{
				for (i = 0, client = level.clients; i < level.maxclients; ++i, ++client)
				{
					if (client->pers.connected != CON_CONNECTED)//client->sess.sessionTeam
						continue;
					if (client->sess.sessionTeam != TEAM_BLUE)
						SetTeam(&g_entities[i], "blue", qtrue);
				}
			}
			else
			{
				if (g_entities[clientid].client->sess.sessionTeam != TEAM_BLUE) {
					SetTeam(&g_entities[clientid], "blue", qtrue);
					trap_SendServerCommand(-1, va("print \"%s ^7has been forced to the ^4Blue ^7team.\n\"", g_entities[clientid].client->pers.netname));
				}
			}
		}
		else if (!Q_stricmp(teamname, "s") || !Q_stricmp(teamname, "spectator") || !Q_stricmp(teamname, "spec") || !Q_stricmp(teamname, "spectate"))
		{
			if (everyone)
			{
				for (i = 0, client = level.clients; i < level.maxclients; ++i, ++client)
				{
					if (client->pers.connected != CON_CONNECTED)//client->sess.sessionTeam
						continue;
					if (client->sess.sessionTeam != TEAM_SPECTATOR)
						SetTeam(&g_entities[i], "spectator", qtrue);
				}
			}
			else
			{
				if (g_entities[clientid].client->sess.sessionTeam != TEAM_SPECTATOR) {
					SetTeam(&g_entities[clientid], "spectator", qtrue);
					trap_SendServerCommand(-1, va("print \"%s ^7has been forced to the ^3Spectator ^7team.\n\"", g_entities[clientid].client->pers.netname));
				}
			}
		}
		else if (!Q_stricmp(teamname, "f") || !Q_stricmp(teamname, "free") || !Q_stricmp(teamname, "join") || !Q_stricmp(teamname, "j") || !Q_stricmp(teamname, "enter"))
		{
			if (everyone)
			{
				for (i = 0, client = level.clients; i < level.maxclients; ++i, ++client)
				{
					if (client->pers.connected != CON_CONNECTED)//client->sess.sessionTeam
						continue;
					if (client->sess.sessionTeam != TEAM_FREE)
						SetTeam(&g_entities[i], "free", qtrue);
				}
			}
			else
			{
				if (g_entities[clientid].client->sess.sessionTeam != TEAM_FREE) {
					SetTeam(&g_entities[clientid], "free", qtrue);
					trap_SendServerCommand(-1, va("print \"%s ^7has been forced to the ^2Free ^7team.\n\"", g_entities[clientid].client->pers.netname));
				}
			}
		}
		else
		{
			trap_SendServerCommand(ent - g_entities, "print \"Usage: amForceTeam <client> <team>\n\"");
		}
	}
	else
	{
		trap_SendServerCommand(ent - g_entities, "print \"You can not use this command in this gametype (amForceTeam).\n\"");
		return;
	}
}
//[JAPRO - Serverside - All - Amforceteam Function - End]

//[JAPRO - Serverside - All - Ampsay Function - Start]
/*
=================
Cmd_Ampsay_f

=================
*/
void Cmd_Ampsay_f(gentity_t *ent)
{
	int pos = 0;
	char real_msg[MAX_STRING_CHARS];
	char *msg = ConcatArgs(1);

	if (!G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_CSPRINT, qfalse, qfalse, "amPsay"))
		return;

	while (*msg)
	{
		if (msg[0] == '\\' && msg[1] == 'n')
		{
			msg++;           // \n is 2 chars, so increase by one here. (one, cuz it's increased down there again...) 
			real_msg[pos++] = '\n';  // put in a real \n 
		}
		else
		{
			real_msg[pos++] = *msg;  // otherwise just copy 
		}
		msg++;                         // increase the msg pointer 
	}
	real_msg[pos] = 0;

	if (trap_Argc() < 2)
	{
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /amPsay <message>.\n\"");
		return;
	}

	trap_SendServerCommand(-1, va("cp \"%s\"", real_msg));

}
//[JAPRO - Serverside - All - Ampsay Function - End]

/*
=================
Cmd_Amfreeze_f

=================
*/
void Cmd_Amfreeze_f(gentity_t *ent)
{
	gentity_t * targetplayer;
	int i;
	int clientid = -1;
	char   arg[MAX_NETNAME];

	if (!G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_FREEZE, qfalse, qfalse, "amFreeze"))
		return;

	if (trap_Argc() != 2)
	{
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /amFreeze <client> or /amFreeze +all or /amFreeze -all.\n\"");
		return;
	}

	trap_Argv(1, arg, sizeof(arg));
	if (Q_stricmp(arg, "+all") == 0)
	{
		for (i = 0; i < level.maxclients; i++)
		{
			targetplayer = &g_entities[i];

			if (targetplayer->client && targetplayer->client->pers.connected)
			{
				if (!G_AdminUsableOn(ent->client, targetplayer->client, JAPRO_ACCOUNTFLAG_A_FREEZE))
					continue;
				if (!targetplayer->client->pers.amfreeze)
					targetplayer->client->pers.amfreeze = qtrue;
				G_ScreenShake(targetplayer->client->ps.origin, &g_entities[i], 3.0f, 2000, qtrue);
				G_Sound(targetplayer, CHAN_AUTO, G_SoundIndex("sound/ambience/thunder_close1"));
			}
		}
		trap_SendServerCommand(-1, "cp \"You have all been frozen\n\"");
		return;
	}
	else if (Q_stricmp(arg, "-all") == 0)
	{
		for (i = 0; i < level.maxclients; i++)
		{
			targetplayer = &g_entities[i];

			if (targetplayer->client && targetplayer->client->pers.connected)
			{
				if (!G_AdminUsableOn(ent->client, g_entities[clientid].client, JAPRO_ACCOUNTFLAG_A_FREEZE))
					continue;
				if (targetplayer->client->pers.amfreeze)
				{
					targetplayer->client->pers.amfreeze = qfalse;
					targetplayer->client->ps.saberCanThrow = qtrue;
					targetplayer->client->ps.forceRestricted = qfalse;
					targetplayer->takedamage = qtrue;
				}
			}
		}
		trap_SendServerCommand(-1, "cp \"You have all been unfrozen\n\"");
		return;
	}
	else
	{
		clientid = JP_ClientNumberFromString(ent, arg);
	}

	if (clientid == -1 || clientid == -2)
	{
		return;
	}

	if (!G_AdminUsableOn(ent->client, g_entities[clientid].client, JAPRO_ACCOUNTFLAG_A_FREEZE)) {
		if (g_entities[clientid].client->ps.clientNum != ent->client->ps.clientNum)
			return;
		else
			trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command on this player (amFreeze).\n\"");
	}

	if (g_entities[clientid].client->noclip == qtrue)
	{
		g_entities[clientid].client->noclip = qfalse;
	}

	if (g_entities[clientid].client->pers.amfreeze)
	{
		g_entities[clientid].client->pers.amfreeze = qfalse;
		g_entities[clientid].client->ps.saberCanThrow = qtrue;
		g_entities[clientid].client->ps.forceRestricted = qfalse;
		g_entities[clientid].takedamage = qtrue;
		trap_SendServerCommand(clientid, "cp \"You have been unfrozen\n\"");
	}
	else
	{
		g_entities[clientid].client->pers.amfreeze = qtrue;
		trap_SendServerCommand(clientid, "cp \"You have been frozen\n\"");
		G_ScreenShake(g_entities[clientid].client->ps.origin, &g_entities[clientid], 3.0f, 2000, qtrue);
		G_Sound(&g_entities[clientid], CHAN_AUTO, G_SoundIndex("sound/ambience/thunder_close1"));
	}
	  }
//[JAPRO - Serverside - All - Amfreeze Function - End]
//[JAPRO - Serverside - All - Amkick Function - Start]
/*
==================
Cmd_Amkick_f

==================
*/
void Cmd_Amkick_f(gentity_t *ent)
{
	int clientid = -1;
	char   arg[MAX_NETNAME];

	if (!G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_ADMINKICK, qfalse, qfalse, "amKick"))
		return;

	if (trap_Argc() != 2)
	{
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /amKick <client>.\n\"");
		return;
	}

	trap_Argv(1, arg, sizeof(arg));
	clientid = JP_ClientNumberFromString(ent, arg);

	if (clientid == -1 || clientid == -2)
	{
		return;
	}

	if (!G_AdminUsableOn(ent->client, g_entities[clientid].client, JAPRO_ACCOUNTFLAG_A_ADMINKICK)) {
		if (g_entities[clientid].client->ps.clientNum != ent->client->ps.clientNum)
			return;
		else
			trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command on this player (amKick).\n\"");
	}

	trap_SendConsoleCommand(EXEC_APPEND, va("clientkick %i", clientid));

}
//[JAPRO - Serverside - All - Amkick Function - End]
//[JAPRO - Serverside - All - Amban Function - Start]
/*
==================
Cmd_Amban_f

==================
*/
void Cmd_Amban_f(gentity_t *ent)
{
	int clientid = -1;
	char   arg[MAX_NETNAME];

	if (!G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_ADMINBAN, qfalse, qfalse, "amBan"))
		return;

	if (trap_Argc() != 2)
	{
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /amBan <client>.\n\"");
		return;
	}

	trap_Argv(1, arg, sizeof(arg));
	clientid = JP_ClientNumberFromString(ent, arg);

	if (clientid == -1 || clientid == -2)
	{
		return;
	}

	if (!G_AdminUsableOn(ent->client, g_entities[clientid].client, JAPRO_ACCOUNTFLAG_A_ADMINBAN)) {
		if (g_entities[clientid].client->ps.clientNum != ent->client->ps.clientNum)
			return;
		else
			trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command on this player (amBan).\n\"");
	}

	AddIP(g_entities[clientid].client->sess.IP);
	trap_SendConsoleCommand(EXEC_APPEND, va("clientkick %i", clientid));

}
//[JAPRO - Serverside - All - Amban Function - End]

//[JAPRO - Serverside - All - Amgrantadmin Function - Start]
/*
=================
Cmd_Amgrantadmin_f
=================
*/
void Cmd_Amgrantadmin_f(gentity_t *ent)
{
	char arg[MAX_NETNAME];
	int clientid = -1;
	int args = trap_Argc();

	if (!G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_GRANTADMIN, qfalse, qfalse, "amGrantAdmin"))
		return;

	if (args != 2 && args != 3)
	{
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /amGrantAdmin <client>.\n\"");
		return;
	}

	trap_Argv(1, arg, sizeof(arg));
	clientid = JP_ClientNumberFromString(ent, arg);

	if (clientid == -1 || clientid == -2)
		return;

	if (!g_entities[clientid].client)
		return;

	if (!G_AdminUsableOn(ent->client, g_entities[clientid].client, JAPRO_ACCOUNTFLAG_A_GRANTADMIN)) {
		if (g_entities[clientid].client->ps.clientNum != ent->client->ps.clientNum)
			return;
		else
			trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command on this player (amGrantAdmin).\n\"");
	}

	if (args == 2) {
		int i;
		qboolean added = qfalse;
		for (i = 0; i <= JAPRO_MAX_ADMIN_BITS; i++) {//Loop this 0-22 is admin flags.
			if (jp_juniorAdminLevel.integer & (1 << i)) {
				g_entities[clientid].client->sess.accountFlags |= (1 << i);
				added = qtrue;
			}
		}
		if (added)
			trap_SendServerCommand(clientid, "print \"You have been granted Junior admin privileges.\n\"");
	}
	else if (args == 3) {
		trap_Argv(2, arg, sizeof(arg));
		if (!Q_stricmp(arg, "none")) {
			int i;
			for (i = 0; i <= JAPRO_MAX_ADMIN_BITS; i++) {//Loop this 0-22 is admin flags.
				g_entities[clientid].client->sess.accountFlags &= ~(1 << i);
			}
		}
	}

}
//[JAPRO - Serverside - All - Amgrantadmin Function - End]

//[JAPRO - Serverside - All - Ampsay Function - Start]
/*
=================
Cmd_Ammap_f
=================
*/
void Cmd_Ammap_f(gentity_t *ent)
{
	char    gametype[2];
	int		gtype;
	char    mapname[MAX_MAPNAMELENGTH];

	if (!G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_CHANGEMAP, qfalse, qfalse, "amMap"))
		return;

	if (trap_Argc() != 3)
	{
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /amMap <gametype #> <map>.\n\"");
		return;
	}

	trap_Argv(1, gametype, sizeof(gametype));
	trap_Argv(2, mapname, sizeof(mapname));

	if (strchr(mapname, ';') || strchr(mapname, '\r') || strchr(mapname, '\n'))
	{
		trap_SendServerCommand(ent - g_entities, "print \"Invalid map string.\n\"");
		return;
	}

	if (gametype[0] < '0' && gametype[0] > '8')
	{
		trap_SendServerCommand(ent - g_entities, "print \"Invalid gametype.\n\"");
		return;
	}

	gtype = atoi(gametype);

	{
		char				unsortedMaps[4096], buf[512] = { 0 };
		char*				possibleMapName;
		int					numMaps;
		const unsigned int  MAX_MAPS = 512;
		qboolean found = qfalse;

		numMaps = trap_FS_GetFileList("maps", ".bsp", unsortedMaps, sizeof(unsortedMaps));
		if (numMaps) {
			int len, i;
			if (numMaps > MAX_MAPS)
				numMaps = MAX_MAPS;
			possibleMapName = unsortedMaps;
			for (i = 0; i < numMaps; i++) {
				len = strlen(possibleMapName);
				if (!Q_stricmp(possibleMapName + len - 4, ".bsp"))
					possibleMapName[len - 4] = '\0';
				if (!Q_stricmp(mapname, possibleMapName)) {
					found = qtrue;
				}
				possibleMapName += len + 1;
			}
		}
		if (!found)
			return;
	}

	//if (ent->client->sess.juniorAdmin)//Logged in as junior admin
	trap_SendServerCommand(-1, va("print \"^3Map change triggered by ^7%s\n\"", ent->client->pers.netname));
	G_LogPrintf("Map change triggered by ^7%s\n", ent->client->pers.netname);

	trap_SendConsoleCommand(EXEC_APPEND, va("g_gametype %i\n", gtype));
	trap_SendConsoleCommand(EXEC_APPEND, va("map %s\n", mapname));

}
//[JAPRO - Serverside - All - Ammap Function - End]

//[JAPRO - Serverside - All - Amvstr Function - Start]
/*
=================
Cmd_Amvstr_f
=================
*/
void Cmd_Amvstr_f(gentity_t *ent)
{
	char   arg[MAX_STRING_CHARS], buf[MAX_CVAR_VALUE_STRING];;

	if (!G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_VSTR, qfalse, qfalse, "amVstr"))
		return;

	if (trap_Argc() != 2)
	{
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /amVstr <vstr>.\n\"");
		return;
	}

	trap_Argv(1, arg, sizeof(arg));

	if (strchr(arg, ';') || strchr(arg, '\r') || strchr(arg, '\n'))
	{
		trap_SendServerCommand(ent - g_entities, "print \"Invalid vstr string.\n\"");
		return;
	}

	//clean the string?
	Q_strlwr(arg);
	Q_CleanStr(arg, qfalse);

	//Check if vstr exists, if not return qfalse.
	trap_Cvar_VariableStringBuffer(arg, buf, sizeof(buf));
	if (!Q_stricmp(buf, ""))
		return;

	trap_SendServerCommand(-1, va("print \"^3Vstr (%s^3) executed by ^7%s\n\"", arg, ent->client->pers.netname));
	G_LogPrintf("Vstr (%s^7) executed by ^7%s\n", arg, ent->client->pers.netname);
	trap_SendConsoleCommand(EXEC_APPEND, va("vstr %s\n", arg));

}
//[JAPRO - Serverside - All - Amvstr Function - End]

typedef struct mapname_s {
	const char	*name;
} mapname_t;

int mapnamecmp(const void *a, const void *b) {
	return Q_stricmp((const char *)a, ((mapname_t*)b)->name);
}

mapname_t defaultMaps[] = {
	{ "ctf_bespin" },
	{ "ctf_imperial" },
	{ "ctf_ns_streets" },
	{ "ctf_yavin" },
	{ "duel_bay" },
	{ "duel_carbon" },
	{ "duel_jedi" },
	{ "duel_pit" },
	{ "ffa_bespin" },
	{ "ffa_deathstar" },
	{ "ffa_imperial" },
	{ "ffa_ns_hideout" },
	{ "ffa_raven" },
	{ "ffa_yavin" },
	{ "pit" }
};
static const size_t numDefaultMaps = ARRAY_LEN(defaultMaps);

qboolean IsBaseMap(char *s) {
	if ((mapname_t *) bsearch(s, defaultMaps, numDefaultMaps, sizeof(defaultMaps[0]), mapnamecmp))
		return qtrue;
	return qfalse;
}

int compcstr(const void * a, const void * b) {
	const char * aa = *(const char * *)a;
	const char * bb = *(const char * *)b;
	return strcmp(aa, bb);
}

void Cmd_AmMapList_f(gentity_t *ent)
{
	char				unsortedMaps[4096], buf[512] = { 0 };
	char*				mapname;
	char*				sortedMaps[512];
	int					i, len, numMaps;
	unsigned int		count = 0, baseMapCount = 0;
	const unsigned int limit = 192, MAX_MAPS = 512;

	if (!G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_LISTMAPS, qfalse, qfalse, "amListMaps"))
		return;

	numMaps = trap_FS_GetFileList("maps", ".bsp", unsortedMaps, sizeof(unsortedMaps));
	if (numMaps) {
		if (numMaps > MAX_MAPS)
			numMaps = MAX_MAPS;
		mapname = unsortedMaps;
		for (i = 0; i < numMaps; i++) {
			len = strlen(mapname);
			if (!Q_stricmp(mapname + len - 4, ".bsp"))
				mapname[len - 4] = '\0';
			sortedMaps[i] = mapname;//String_Alloc(mapname);
			mapname += len + 1;
		}

		qsort(sortedMaps, numMaps, sizeof(sortedMaps[0]), compcstr);
		Q_strncpyz(buf, "   ", sizeof(buf));

		for (i = 0; i < numMaps; i++) {
			char *tmpMsg = NULL;

			if (IsBaseMap(sortedMaps[i])) { //Ideally this could be done before the qsort, but that likes to crash it since it changes the array size or something
				baseMapCount++;
				continue;
			}
			tmpMsg = va(" ^3%-32s    ", sortedMaps[i]);
			if (count >= limit) {
				tmpMsg = va("\n   %s", tmpMsg);
				count = 0;
			}
			if (strlen(buf) + strlen(tmpMsg) >= sizeof(buf)) {
				trap_SendServerCommand(ent - g_entities, va("print \"%s\"", buf));
				buf[0] = '\0';
			}
			count += strlen(tmpMsg);
			Q_strcat(buf, sizeof(buf), tmpMsg);
		}
		trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", buf));
		trap_SendServerCommand(ent - g_entities, va("print \"^5%i maps listed\n\"", numMaps - baseMapCount));
	}
}

//amkillvote from raz0r start
static void Cmd_Amkillvote_f(gentity_t *ent) {

	if (!G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_KILLVOTE, qfalse, qfalse, "amKillVote"))
		return;

	if (level.voteTime) //there is a vote in progress
		trap_SendServerCommand(-1, "print \"" S_COLOR_RED "Vote has been killed!\n\"");

	//Overkill, but it's a surefire way to kill the vote =]
	level.voteExecuteTime = 0;
	level.votingGametype = qfalse;
	level.votingGametypeTo = g_gametype.integer;
	level.voteTime = 0;

	level.voteDisplayString[0] = '\0';
	level.voteString[0] = '\0';

	trap_SetConfigstring(CS_VOTE_TIME, "");
	trap_SetConfigstring(CS_VOTE_STRING, "");
	trap_SetConfigstring(CS_VOTE_YES, "");
	trap_SetConfigstring(CS_VOTE_NO, "");
}
//amkillvote end

//[JAPRO - Serverside - All - Aminfo Function - Start]
/*
=================
Cmd_Aminfo_f
=================
*/
void Cmd_Aminfo_f(gentity_t *ent)
{
	char buf[MAX_STRING_CHARS - 64] = { 0 };

	if (!ent || !ent->client)
		return;

	Q_strncpyz(buf, va("^5 Hi there, %s^5. This server is using the jk2PRO mod.\n", ent->client->pers.netname), sizeof(buf));
	Q_strcat(buf, sizeof(buf), "   ^3To display server settings, type ^7serverConfig");
	trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", buf));

	/*Q_strncpyz(buf, "   ^3Account commands: ", sizeof(buf));
	Q_strcat(buf, sizeof(buf), "register ");
	Q_strcat(buf, sizeof(buf), "login ");
	Q_strcat(buf, sizeof(buf), "logout ");
	Q_strcat(buf, sizeof(buf), "changepassword ");
	Q_strcat(buf, sizeof(buf), "stats ");
	Q_strcat(buf, sizeof(buf), "top ");
	Q_strcat(buf, sizeof(buf), "whois");
	trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", buf));

	if (g_allowRegistration.integer > 1) {
		Q_strncpyz(buf, "   ^3Clan commands: ", sizeof(buf));
		Q_strcat(buf, sizeof(buf), "clanList ");
		Q_strcat(buf, sizeof(buf), "clanInfo ");
		Q_strcat(buf, sizeof(buf), "clanJoin ");
		Q_strcat(buf, sizeof(buf), "clanAdmin ");
		if (g_allowRegistration.integer > 2)
			Q_strcat(buf, sizeof(buf), "clanCreate ");
		Q_strcat(buf, sizeof(buf), "clanInvite");
		trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", buf));
	}
	*/
	Q_strncpyz(buf, "   ^3Chat commands: ", sizeof(buf));
	//Q_strcat(buf, sizeof(buf), "ignore ");
	//Q_strcat(buf, sizeof(buf), "clanPass ");
	//Q_strcat(buf, sizeof(buf), "clanWhoIs ");
	//Q_strcat(buf, sizeof(buf), "clanSay ");
	Q_strcat(buf, sizeof(buf), "amSay ");
	//Q_strcat(buf, sizeof(buf), "say_team_mod");
	trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", buf));

	Q_strncpyz(buf, "   ^3Game commands: ", sizeof(buf));
	Q_strcat(buf, sizeof(buf), "amMOTD ");
	Q_strcat(buf, sizeof(buf), "printStats ");
	Q_strcat(buf, sizeof(buf), "showNet ");
	if (g_privateDuel.integer) {
		Q_strcat(buf, sizeof(buf), "engage_FullForceDuel ");
		Q_strcat(buf, sizeof(buf), "engage_gunDuel ");
	}
	/*if (g_allowSaberSwitch.integer)
		Q_strcat(buf, sizeof(buf), "saber ");
	if (g_allowFlagThrow.integer && ((g_gametype.integer == GT_CTF) || g_rabbit.integer))
		Q_strcat(buf, sizeof(buf), "throwFlag ");
	if (g_allowTargetLaser.integer)
		Q_strcat(buf, sizeof(buf), "+button15 (target laser) ");
	if ((g_gametype.integer >= GT_TEAM) && g_allowSpotting.integer)
		Q_strcat(buf, sizeof(buf), "spot ");
	if (g_allowGrapple.integer)
		Q_strcat(buf, sizeof(buf), "+button12 (grapple) ");
	if (g_tweakJetpack.integer)
		Q_strcat(buf, sizeof(buf), "double jump (jetpack) ");
	trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", buf));*/

	/*if (jp_raceMode.integer) {
		Q_strncpyz(buf, "   ^3Defrag commands: ", sizeof(buf));
		Q_strcat(buf, sizeof(buf), "rTop ");
		Q_strcat(buf, sizeof(buf), "rLatest ");
		Q_strcat(buf, sizeof(buf), "rRank ");
		Q_strcat(buf, sizeof(buf), "rWorst ");
		Q_strcat(buf, sizeof(buf), "rHardest ");
		Q_strcat(buf, sizeof(buf), "rPopular ");
		if (jp_raceMode.integer > 1)
			Q_strcat(buf, sizeof(buf), "race ");
		Q_strcat(buf, sizeof(buf), "jump ");
		Q_strcat(buf, sizeof(buf), "move ");
		Q_strcat(buf, sizeof(buf), "rocketChange ");
		Q_strcat(buf, sizeof(buf), "hide ");
		Q_strcat(buf, sizeof(buf), "practice ");
		Q_strcat(buf, sizeof(buf), "launch ");
		Q_strcat(buf, sizeof(buf), "ysal ");
		Q_strcat(buf, sizeof(buf), "warpList ");
		Q_strcat(buf, sizeof(buf), "warp ");
		if (jp_allowRaceTele.integer) {
			Q_strcat(buf, sizeof(buf), "amTele ");
			Q_strcat(buf, sizeof(buf), "amTelemark ");
			if (jp_allowRaceTele.integer > 1)
				Q_strcat(buf, sizeof(buf), "noclip ");
		}
		Q_strcat(buf, sizeof(buf), "+button13 (dodge/dash/walljump)");
		trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", buf));
	}*/

	Q_strncpyz(buf, "   ^3Emote commands: ", sizeof(buf));
	if (!(jp_emotesDisable.integer & (1 << E_BEG)))
		Q_strcat(buf, sizeof(buf), "amBeg ");
	if (!(jp_emotesDisable.integer & (1 << E_BEG2)))
		Q_strcat(buf, sizeof(buf), "amBeg2 ");
	if (!(jp_emotesDisable.integer & (1 << E_BREAKDANCE)))
		Q_strcat(buf, sizeof(buf), "amBreakdance[2/3/4] ");
	if (!(jp_emotesDisable.integer & (1 << E_DANCE)))
		Q_strcat(buf, sizeof(buf), "amDance ");
	if (!(jp_emotesDisable.integer & (1 << E_HUG)))
		Q_strcat(buf, sizeof(buf), "amHug ");
	if (!(jp_emotesDisable.integer & (1 << E_SABERFLIP)))
		Q_strcat(buf, sizeof(buf), "amFlip ");
	if (!(jp_emotesDisable.integer & (1 << E_POINT)))
		Q_strcat(buf, sizeof(buf), "amPoint ");
	if (!(jp_emotesDisable.integer & (1 << E_SIT)))
		Q_strcat(buf, sizeof(buf), "amSit[2/3/4/5] ");
	if (!(jp_emotesDisable.integer & (1 << E_SLAP)))
		Q_strcat(buf, sizeof(buf), "amSlap ");
	if (!(jp_emotesDisable.integer & (1 << E_SLEEP)))
		Q_strcat(buf, sizeof(buf), "amSleep ");
	if (!(jp_emotesDisable.integer & (1 << E_SURRENDER)))
		Q_strcat(buf, sizeof(buf), "amSurrender ");
	if (!(jp_emotesDisable.integer & (1 << E_SMACK)))
		Q_strcat(buf, sizeof(buf), "amSmack ");
	trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", buf));

	Q_strncpyz(buf, "   ^3Admin commands: ", sizeof(buf));
	if (!(ent->client->sess.accountFlags)) //fixme.. idk
		Q_strcat(buf, sizeof(buf), "you are not an administrator on this server.\n");
	else {
		if (G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_ADMINTELE, qfalse, qfalse, NULL))
			Q_strcat(buf, sizeof(buf), "amTele ");
		if (G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_TELEMARK, qfalse, qfalse, NULL))
			Q_strcat(buf, sizeof(buf), "amTeleMark ");
		if (G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_FREEZE, qfalse, qfalse, NULL))
			Q_strcat(buf, sizeof(buf), "amFreeze ");
		if (G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_ADMINBAN, qfalse, qfalse, NULL))
			Q_strcat(buf, sizeof(buf), "amBan ");
		if (G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_ADMINKICK, qfalse, qfalse, NULL))
			Q_strcat(buf, sizeof(buf), "amKick ");
		if (G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_KILLVOTE, qfalse, qfalse, NULL))
			Q_strcat(buf, sizeof(buf), "amKillVote ");
		/*if (G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_NPC, qfalse, qfalse, NULL))
			Q_strcat(buf, sizeof(buf), "NPC ");*/
		if (G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_NOCLIP, qfalse, qfalse, NULL))
			Q_strcat(buf, sizeof(buf), "Noclip ");
		if (G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_ADMINTELE, qfalse, qfalse, NULL))
			Q_strcat(buf, sizeof(buf), "amTele ");
		if (G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_GRANTADMIN, qfalse, qfalse, NULL))
			Q_strcat(buf, sizeof(buf), "amGrantAdmin ");
		if (G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_CHANGEMAP, qfalse, qfalse, NULL))
			Q_strcat(buf, sizeof(buf), "amMap ");
		if (G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_LISTMAPS, qfalse, qfalse, NULL))
			Q_strcat(buf, sizeof(buf), "amListMaps ");
		if (G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_CSPRINT, qfalse, qfalse, NULL))
			Q_strcat(buf, sizeof(buf), "amPsay ");
		if (G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_FORCETEAM, qfalse, qfalse, NULL))
			Q_strcat(buf, sizeof(buf), "amForceTeam ");
		if (G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_LOOKUP, qfalse, qfalse, NULL))
			Q_strcat(buf, sizeof(buf), "amLookup ");
		if (G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_VSTR, qfalse, qfalse, NULL))
			Q_strcat(buf, sizeof(buf), "amVstr ");
		if (G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_RENAME, qfalse, qfalse, NULL))
			Q_strcat(buf, sizeof(buf), "amRename ");
		trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", buf));
		buf[0] = '\0';
	}

	if (ent->client->pers.isJK2PRO)
		trap_SendServerCommand(ent - g_entities, "print \"   ^2You are using the client plugin recommended by the server.\n\"");
	else //burnie.com is completely temporary, because I don't have a website yet.  Thanks Mr. Burns.
		trap_SendServerCommand(ent - g_entities, "print \"   ^1You do not have the client plugin. Download at burnie.com\n\"");

}

static void Cmd_Amlookup_f(gentity_t *ent)
{//Display list of players + clientNum + IP + admin
	int              fLen = 0, clientid;
	char             msg[1024 - 128] = { 0 }, buf[60 * 512], strIP[NET_ADDRSTRMAXLEN] = { 0 };
	char*	pch;
	char *p = NULL;
	char last[64], client[MAX_NETNAME];
	fileHandle_t f;
	qboolean multiple = qfalse;

	if (!G_AdminAllowed(ent, JAPRO_ACCOUNTFLAG_A_LOOKUP, qfalse, qfalse, "amLookup"))
		return;

	if (trap_Argc() != 2) {
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /amLookup <client>\n\"");
		return;
	}

	trap_Argv(1, client, sizeof(client));
	clientid = JP_ClientNumberFromString(ent, client);

	if (clientid == -1 || clientid == -2)
		return;

	fLen = trap_FS_FOpenFile(PLAYER_LOG, &f, FS_READ);
	if (!f) {
		Com_Printf("ERROR: Couldn't load player logfile %s\n", PLAYER_LOG);
		return;
	}

	if (fLen >= 80 * 1024) {
		trap_FS_FCloseFile(f);
		Com_Printf("ERROR: Couldn't load player logfile %s, file is too large\n", PLAYER_LOG);
		return;
	}

	Q_strncpyz(strIP, g_entities[clientid].client->sess.IP, sizeof(strIP));

	p = strchr(strIP, ':');
	if (p) //loda - fix ip sometimes not printing
		*p = 0;

	trap_FS_Read(buf, fLen, f);
	buf[fLen] = 0;
	trap_FS_FCloseFile(f);

	pch = strtok(buf, ";\n");

	Q_strncpyz(last, pch, sizeof(last));

	while (pch != NULL) {
		if (!Q_stricmp(strIP, pch)) {

			if (multiple) {
				Q_strcat(msg, sizeof(msg), va("\n  ^7%s", last));
			}
			else {
				Q_strcat(msg, sizeof(msg), va("^7%s", last));
			}
			multiple = qtrue;
		}
		Q_strncpyz(last, pch, sizeof(last));
		pch = strtok(NULL, ";\n");
	}
	trap_SendServerCommand(ent - g_entities, va("print \"^5 This players IP has used the following names on this server:\n  %s\n\"", msg));
}

void PM_SetAnim(int setAnimParts,int anim,int setAnimFlags, int blendTime);

#ifdef _DEBUG
extern stringID_table_t animTable[MAX_ANIMATIONS+1];

void Cmd_DebugSetSaberMove_f(gentity_t *self)
{
	int argNum = trap_Argc();
	char arg[MAX_STRING_CHARS];

	if (argNum < 2)
	{
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );

	if (!arg[0])
	{
		return;
	}

	self->client->ps.saberMove = atoi(arg);
	self->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;

	if (self->client->ps.saberMove >= LS_MOVE_MAX)
	{
		self->client->ps.saberMove = LS_MOVE_MAX-1;
	}

	Com_Printf("Anim for move: %s\n", animTable[saberMoveData[self->client->ps.saberMove].animToUse].name);
}

void Cmd_DebugSetBodyAnim_f(gentity_t *self, int flags)
{
	int argNum = trap_Argc();
	char arg[MAX_STRING_CHARS];
	int i = 0;
	pmove_t pmv;

	if (argNum < 2)
	{
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );

	if (!arg[0])
	{
		return;
	}

	while (i < MAX_ANIMATIONS)
	{
		if (!Q_stricmp(arg, animTable[i].name))
		{
			break;
		}
		i++;
	}

	if (i == MAX_ANIMATIONS)
	{
		Com_Printf("Animation '%s' does not exist\n", arg);
		return;
	}

	memset (&pmv, 0, sizeof(pmv));
	pmv.ps = &self->client->ps;
	pmv.animations = bgGlobalAnimations;
	pmv.cmd = self->client->pers.cmd;
	pmv.trace = trap_Trace;
	pmv.pointcontents = trap_PointContents;
	pmv.gametype = g_gametype.integer;

	pm = &pmv;
	PM_SetAnim(SETANIM_BOTH, i, flags, 0);

	Com_Printf("Set body anim to %s\n", arg);
}
#endif

void StandardSetBodyAnim(gentity_t *self, int anim, int flags, int body)
{
	pmove_t pmv;

	memset (&pmv, 0, sizeof(pmv));
	pmv.ps = &self->client->ps;
	pmv.animations = bgGlobalAnimations;
	pmv.cmd = self->client->pers.cmd;
	pmv.trace = trap_Trace;
	pmv.pointcontents = trap_PointContents;
	pmv.gametype = g_gametype.integer;

	pm = &pmv;
	//PM_SetAnim(SETANIM_BOTH, anim, flags, 0);
	PM_SetAnim(body, anim, flags, 0);
}

static void DoEmote(gentity_t *ent, int anim, qboolean freeze, qboolean nosaber, int body)
{
	if (!ent->client)
		return;
	if (ent->client->ps.weaponTime > 1 || ent->client->ps.saberMove > 1 || ent->client->ps.fd.forcePowersActive)
		return;
	if (ent->client->ps.groundEntityNum == ENTITYNUM_NONE) //Not on ground
		return;
	if (ent->client->ps.duelInProgress) {
		trap_SendServerCommand(ent - g_entities, "print \"^7Emotes not allowed in duel!\n\"");
		return;
	}
	/*if (BG_InKnockDown(ent->s.legsAnim))
	return;*/
	if (BG_InRoll(&ent->client->ps, ent->s.legsAnim))//is this crashing? if ps is null or something?
		return;
	if (ent->client->sess.raceMode) {//No emotes in racemode i guess
		trap_SendServerCommand(ent - g_entities, "print \"^7Emotes not allowed in racemode!\n\"");
		return;
	}
	if (g_gametype.integer != GT_FFA) {
		trap_SendServerCommand(ent - g_entities, "print \"^7Emotes not allowed in this gametype!\n\"");
		return;
	}

	if (freeze) { // Do the anim and freeze it, or cancel if already in it
		if (ent->client->ps.legsAnim == anim) // Cancel the anim if already in it?
			ent->client->emote_freeze = qfalse;
		else //Do the anim
			ent->client->emote_freeze = qtrue;
	}
	if (nosaber && ent->client->ps.weapon == WP_SABER && ent->client->ps.saberHolstered < 2) {
		ent->client->ps.saberCanThrow = qfalse;
		ent->client->ps.saberMove = LS_NONE;
		ent->client->ps.saberBlocked = 0;
		ent->client->ps.saberBlocking = 0;
		ent->client->ps.saberHolstered = 2;
		/*if (ent->client->saber[0].soundOff)
		G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOff);
		if (ent->client->saber[1].soundOff && ent->client->saber[1].model[0])
		G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOff);*/
	}
	StandardSetBodyAnim(ent, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS, body);
}

static void Cmd_EmoteSit_f(gentity_t *ent)
{
	if (jp_emotesDisable.integer & (1 << E_SIT)) {
		trap_SendServerCommand(ent - g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_SIT1, qtrue, qtrue, SETANIM_BOTH);
	}

static void Cmd_EmoteSit2_f(gentity_t *ent)
{
	if (jp_emotesDisable.integer & (1 << E_SIT)) {
		trap_SendServerCommand(ent - g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_SIT3, qtrue, qtrue, SETANIM_BOTH);
}

static void Cmd_EmoteSit3_f(gentity_t *ent)
{
	if (jp_emotesDisable.integer & (1 << E_SIT)) {
		trap_SendServerCommand(ent - g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_SIT2, qtrue, qtrue, SETANIM_BOTH);
}

static void Cmd_EmoteSurrender_f(gentity_t *ent)
{
	if (jp_emotesDisable.integer & (1 << E_SURRENDER)) {
		trap_SendServerCommand(ent - g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, TORSO_SURRENDER_START, qtrue, qtrue, SETANIM_BOTH);
}

static void Cmd_EmoteSmack_f(gentity_t *ent)
{
	if (jp_emotesDisable.integer & (1 << E_SMACK)) {
		trap_SendServerCommand(ent - g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_TOSS1, qfalse, qfalse, SETANIM_BOTH);
}

static void Cmd_EmoteHug_f(gentity_t *ent)
{
	if (jp_emotesDisable.integer & (1 << E_HUG)) {
		trap_SendServerCommand(ent - g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_HUGGER1, qfalse, qtrue, SETANIM_BOTH);
}

static void Cmd_EmoteBeg_f(gentity_t *ent)
{
	if (jp_emotesDisable.integer & (1 << E_BEG)) {
		trap_SendServerCommand(ent - g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_KNEES1, qtrue, qtrue, SETANIM_BOTH);
}

static void Cmd_EmoteBeg2_f(gentity_t *ent)
{
	if (jp_emotesDisable.integer & (1 << E_BEG2)) {
		trap_SendServerCommand(ent - g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_KNEES2, qtrue, qtrue, SETANIM_BOTH);
}

static void Cmd_EmoteBreakdance_f(gentity_t *ent)
{
	if (jp_emotesDisable.integer & (1 << E_BREAKDANCE)) {
		trap_SendServerCommand(ent - g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_FORCE_GETUP_B2, qfalse, qtrue, SETANIM_BOTH);
}

static void Cmd_EmoteBreakdance2_f(gentity_t *ent)
{
	if (jp_emotesDisable.integer & (1 << E_BREAKDANCE)) {
		trap_SendServerCommand(ent - g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_FORCE_GETUP_B4, qfalse, qtrue, SETANIM_BOTH);
}

static void Cmd_EmoteBreakdance3_f(gentity_t *ent)
{
	if (jp_emotesDisable.integer & (1 << E_BREAKDANCE)) {
		trap_SendServerCommand(ent - g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_FORCE_GETUP_B5, qfalse, qtrue, SETANIM_BOTH);
}

static void Cmd_EmoteBreakdance4_f(gentity_t *ent)
{
	if (jp_emotesDisable.integer & (1 << E_BREAKDANCE)) {
		trap_SendServerCommand(ent - g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_FORCE_GETUP_B6, qfalse, qtrue, SETANIM_BOTH);
}

static void Cmd_EmoteSit4_f(gentity_t *ent)
{
	if (jp_emotesDisable.integer & (1 << E_SIT)) {
		trap_SendServerCommand(ent - g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_SLEEP6STOP, qtrue, qtrue, SETANIM_BOTH);
}

static void Cmd_EmoteSleep_f(gentity_t *ent)
{
	if (jp_emotesDisable.integer & (1 << E_SLEEP)) {
		trap_SendServerCommand(ent - g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_SLEEP1, qtrue, qtrue, SETANIM_BOTH);
}

static void Cmd_EmoteDance_f(gentity_t *ent)
{
	if (jp_emotesDisable.integer & (1 << E_DANCE)) {
		trap_SendServerCommand(ent - g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_STEADYSELF1, qfalse, qtrue, SETANIM_BOTH);
}

static void Cmd_EmotePoint_f(gentity_t *ent)
{
	if (jp_emotesDisable.integer & (1 << E_POINT)) {
		trap_SendServerCommand(ent - g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_STAND5TOAIM, qtrue, qfalse, SETANIM_BOTH);
}

static void Cmd_EmoteSaberFlip_f(gentity_t *ent)
{
	if (!ent->client || ent->client->ps.weapon != WP_SABER) //Dont allow if saber isnt out? eh
		return;

	if (jp_emotesDisable.integer & (1 << E_SABERFLIP)) {
		trap_SendServerCommand(ent - g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}

	//if (ent->client->ps.fd.saberAnimLevel == SS_FAST || ent->client->ps.fd.saberAnimLevel == SS_MEDIUM || ent->client->ps.fd.saberAnimLevel == SS_STRONG) { //Get style, do specific anim
		if (ent->client->ps.saberHolstered)
			DoEmote(ent, BOTH_STAND1TO2, qfalse, qfalse, SETANIM_BOTH);
		else
			DoEmote(ent, BOTH_STAND2TO1, qfalse, qfalse, SETANIM_BOTH);
	//}
	/*
	else if (ent->client->ps.fd.saberAnimLevel == SS_STAFF) {
		if (ent->client->ps.saberHolstered)
			DoEmote(ent, BOTH_S1_S7, qfalse, qfalse, SETANIM_BOTH);
		else
			DoEmote(ent, BOTH_SHOWOFF_STAFF, qfalse, qfalse, SETANIM_BOTH);
	}
	else if (ent->client->ps.fd.saberAnimLevel == SS_DUAL) {
		DoEmote(ent, BOTH_SHOWOFF_FAST, qfalse, qfalse, SETANIM_BOTH);
	}*/

	Cmd_ToggleSaber_f(ent);
}

static void Cmd_EmoteSlap_f(gentity_t *ent)
{
	if (jp_emotesDisable.integer & (1 << E_SLAP)) {
		trap_SendServerCommand(ent - g_entities, "print \"This emote is not allowed on this server.\n\"");
		return;
	}
	DoEmote(ent, BOTH_FORCEGRIP3THROW, qfalse, qfalse, SETANIM_BOTH);
}

void DismembermentTest(gentity_t *self);

#ifdef _DEBUG
void DismembermentByNum(gentity_t *self, int num);
#endif

/*
=================
ClientCommand
=================
*/
void ClientCommand( int clientNum ) {
	gentity_t *ent;
	char	cmd[MAX_TOKEN_CHARS];

	ent = g_entities + clientNum;
	if ( !ent->client || ent->client->pers.connected < CON_CONNECTED ) {
		return;		// not fully in game yet
	}


	trap_Argv( 0, cmd, sizeof( cmd ) );
	
	// Filter "\n" and "\r"
	if( strchr(ConcatArgs(0), '\n') != NULL || strchr(ConcatArgs(0), '\r') != NULL )
	{
		trap_SendServerCommand( clientNum, "print \"Invalid input - command blocked.\n\"" );
		G_Printf("ClientCommand: client '%i' (%s) tried to use an invalid command - command blocked.\n", clientNum, ent->client->pers.netname);
		return;
	}

	//rww - redirect bot commands
	if (strstr(cmd, "bot_") && AcceptBotCommand(cmd, ent))
	{
		return;
	}
	//end rww

	if (Q_stricmp (cmd, "say") == 0) {
		Cmd_Say_f (ent, SAY_ALL, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "say_team") == 0) {
		Cmd_Say_f (ent, SAY_TEAM, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "tell") == 0) {
		Cmd_Tell_f ( ent );
		return;
	}
	/*
	if (Q_stricmp (cmd, "vsay") == 0) {
		Cmd_Voice_f (ent, SAY_ALL, qfalse, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "vsay_team") == 0) {
		Cmd_Voice_f (ent, SAY_TEAM, qfalse, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "vtell") == 0) {
		Cmd_VoiceTell_f ( ent, qfalse );
		return;
	}
	if (Q_stricmp (cmd, "vosay") == 0) {
		Cmd_Voice_f (ent, SAY_ALL, qfalse, qtrue);
		return;
	}
	if (Q_stricmp (cmd, "vosay_team") == 0) {
		Cmd_Voice_f (ent, SAY_TEAM, qfalse, qtrue);
		return;
	}
	if (Q_stricmp (cmd, "votell") == 0) {
		Cmd_VoiceTell_f ( ent, qtrue );
		return;
	}
	if (Q_stricmp (cmd, "vtaunt") == 0) {
		Cmd_VoiceTaunt_f ( ent );
		return;
	}
	*/
	if (Q_stricmp (cmd, "score") == 0) {
		Cmd_Score_f (ent);
		return;
	}

	// ignore all other commands when at intermission
	if (level.intermissiontime)
	{
		qboolean giveError = qfalse;

		if (!Q_stricmp(cmd, "give"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "god"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "notarget"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "noclip"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "kill"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "teamtask"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "levelshot"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "follow"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "follownext"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "followprev"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "team"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "forcechanged"))
		{ //special case: still update force change
			Cmd_ForceChanged_f (ent);
			return;
		}
		else if (!Q_stricmp(cmd, "where"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "callvote"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "vote"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "callteamvote"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "teamvote"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "gc"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "setviewpos"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "stats"))
		{
			giveError = qtrue;
		}

		if (giveError)
		{
			trap_SendServerCommand( clientNum, va("print \"You cannot perform this task (%s) during the intermission.\n\"", cmd ) );
		}
		else
		{
			Cmd_Say_f (ent, qfalse, qtrue);
		}
		return;
	}

	if (Q_stricmp (cmd, "give") == 0)
	{
		Cmd_Give_f (ent);
	}
	else if (Q_stricmp (cmd, "god") == 0)
		Cmd_God_f (ent);
	else if (Q_stricmp (cmd, "notarget") == 0)
		Cmd_Notarget_f (ent);
	else if (Q_stricmp (cmd, "noclip") == 0)
		Cmd_Noclip_f (ent);
	else if (Q_stricmp (cmd, "kill") == 0)
		Cmd_Kill_f (ent);
	else if (Q_stricmp (cmd, "teamtask") == 0)
		Cmd_TeamTask_f (ent);
	else if (Q_stricmp (cmd, "levelshot") == 0)
		Cmd_LevelShot_f (ent);
	else if (Q_stricmp (cmd, "follow") == 0)
		Cmd_Follow_f (ent);
	else if (Q_stricmp (cmd, "follownext") == 0)
		Cmd_FollowCycle_f (ent, 1);
	else if (Q_stricmp (cmd, "followprev") == 0)
		Cmd_FollowCycle_f (ent, -1);
	else if (Q_stricmp (cmd, "team") == 0)
		Cmd_Team_f (ent);
	else if (Q_stricmp (cmd, "forcechanged") == 0)
		Cmd_ForceChanged_f (ent);
	else if (Q_stricmp (cmd, "where") == 0)
		Cmd_Where_f (ent);
	else if (Q_stricmp (cmd, "callvote") == 0)
		Cmd_CallVote_f (ent);
	else if (Q_stricmp (cmd, "vote") == 0)
		Cmd_Vote_f (ent);
	else if (Q_stricmp (cmd, "callteamvote") == 0)
		Cmd_CallTeamVote_f (ent);
	else if (Q_stricmp (cmd, "teamvote") == 0)
		Cmd_TeamVote_f (ent);
	else if (Q_stricmp (cmd, "gc") == 0)
		Cmd_GameCommand_f( ent );
	else if (Q_stricmp (cmd, "setviewpos") == 0)
		Cmd_SetViewpos_f( ent );
	else if (Q_stricmp (cmd, "stats") == 0)
		Cmd_Stats_f( ent );
	/*
	else if (Q_stricmp(cmd, "#mm") == 0 && CheatsOk( ent ))
	{
		G_PlayerBecomeATST(ent);
	}
	*/
	//I broke the ATST when I restructured it to use a single global anim set for all client animation.
	//You can fix it, but you'll have to implement unique animations (per character) again.
#ifdef _DEBUG //sigh..
	else if (Q_stricmp(cmd, "headexplodey") == 0 && CheatsOk( ent ))
	{
		Cmd_Kill_f (ent);
		if (ent->health < 1)
		{
			float presaveVel = ent->client->ps.velocity[2];
			ent->client->ps.velocity[2] = 500;
			DismembermentTest(ent);
			ent->client->ps.velocity[2] = presaveVel;
		}
	}
	else if (Q_stricmp(cmd, "g2animent") == 0 && CheatsOk( ent ))
	{
		G_CreateExampleAnimEnt(ent);
	}
	else if (Q_stricmp(cmd, "loveandpeace") == 0 && CheatsOk( ent ))
	{
		trace_t tr;
		vec3_t fPos;

		AngleVectors(ent->client->ps.viewangles, fPos, 0, 0);

		fPos[0] = ent->client->ps.origin[0] + fPos[0]*40;
		fPos[1] = ent->client->ps.origin[1] + fPos[1]*40;
		fPos[2] = ent->client->ps.origin[2] + fPos[2]*40;

		trap_Trace(&tr, ent->client->ps.origin, 0, 0, fPos, ent->s.number, ent->clipmask);

		if (tr.entityNum < MAX_CLIENTS && tr.entityNum != ent->s.number)
		{
			gentity_t *other = &g_entities[tr.entityNum];

			if (other && other->inuse && other->client)
			{
				vec3_t entDir;
				vec3_t otherDir;
				vec3_t entAngles;
				vec3_t otherAngles;

				if (ent->client->ps.weapon == WP_SABER && !ent->client->ps.saberHolstered)
				{
					Cmd_ToggleSaber_f(ent);
				}

				if (other->client->ps.weapon == WP_SABER && !other->client->ps.saberHolstered)
				{
					Cmd_ToggleSaber_f(other);
				}

				if ((ent->client->ps.weapon != WP_SABER || ent->client->ps.saberHolstered) &&
					(other->client->ps.weapon != WP_SABER || other->client->ps.saberHolstered))
				{
					VectorSubtract( other->client->ps.origin, ent->client->ps.origin, otherDir );
					VectorCopy( ent->client->ps.viewangles, entAngles );
					entAngles[YAW] = vectoyaw( otherDir );
					SetClientViewAngle( ent, entAngles );

					StandardSetBodyAnim(ent, BOTH_KISSER1LOOP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS);
					ent->client->ps.saberMove = LS_NONE;
					ent->client->ps.saberBlocked = 0;
					ent->client->ps.saberBlocking = 0;

					VectorSubtract( ent->client->ps.origin, other->client->ps.origin, entDir );
					VectorCopy( other->client->ps.viewangles, otherAngles );
					otherAngles[YAW] = vectoyaw( entDir );
					SetClientViewAngle( other, otherAngles );

					StandardSetBodyAnim(other, BOTH_KISSEE1LOOP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS);
					other->client->ps.saberMove = LS_NONE;
					other->client->ps.saberBlocked = 0;
					other->client->ps.saberBlocking = 0;
				}
			}
		}
	}
#endif
	else if (Q_stricmp(cmd, "thedestroyer") == 0 && CheatsOk( ent ) && ent && ent->client && ent->client->ps.saberHolstered && ent->client->ps.weapon == WP_SABER)
	{
		Cmd_ToggleSaber_f(ent);

		if (!ent->client->ps.saberHolstered)
		{
			if (ent->client->ps.dualBlade)
			{
				ent->client->ps.dualBlade = qfalse;
				//ent->client->ps.fd.saberAnimLevel = FORCE_LEVEL_1;
			}
			else
			{
				ent->client->ps.dualBlade = qtrue;

				trap_SendServerCommand( -1, va("print \"%sTHE DESTROYER COMETH\n\"", S_COLOR_RED) );
				G_ScreenShake(vec3_origin, NULL, 10.0f, 800, qtrue);
				//ent->client->ps.fd.saberAnimLevel = FORCE_LEVEL_3;
			}
		}
	}
#ifdef _DEBUG
	else if (Q_stricmp(cmd, "debugSetSaberMove") == 0)
	{
		Cmd_DebugSetSaberMove_f(ent);
	}
	else if (Q_stricmp(cmd, "debugSetBodyAnim") == 0)
	{
		Cmd_DebugSetBodyAnim_f(ent, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
	}
	else if (Q_stricmp(cmd, "debugDismemberment") == 0)
	{
		Cmd_Kill_f (ent);
		if (ent->health < 1)
		{
			char	arg[MAX_STRING_CHARS];
			int		iArg = 0;

			if (trap_Argc() > 1)
			{
				trap_Argv( 1, arg, sizeof( arg ) );

				if (arg[0])
				{
					iArg = atoi(arg);
				}
			}

			DismembermentByNum(ent, iArg);
		}
	}
	else if (Q_stricmp(cmd, "debugKnockMeDown") == 0)
	{
		ent->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
		ent->client->ps.forceDodgeAnim = 0;
		if (trap_Argc() > 1)
		{
			ent->client->ps.forceHandExtendTime = level.time + 1100;
			ent->client->ps.quickerGetup = qfalse;
		}
		else
		{
			ent->client->ps.forceHandExtendTime = level.time + 700;
			ent->client->ps.quickerGetup = qtrue;
		}
	}
#endif
	else if (Q_stricmp(cmd, "aminfo") == 0) {
		Cmd_Aminfo_f(ent);
	}
	else if (Q_stricmp(cmd, "ambeg") == 0) {
		Cmd_EmoteBeg_f(ent);
	}
	else if (Q_stricmp(cmd, "ambeg2") == 0) {
		Cmd_EmoteBeg2_f(ent);
	}
	else if (Q_stricmp(cmd, "ambreakdance") == 0) {
		Cmd_EmoteBreakdance_f(ent);
	}
	else if (Q_stricmp(cmd, "ambreakdance2") == 0) {
		Cmd_EmoteBreakdance2_f(ent);
	}
	else if (Q_stricmp(cmd, "ambreakdance3") == 0) {
		Cmd_EmoteBreakdance3_f(ent);
	}
	else if (Q_stricmp(cmd, "ambreakdance4") == 0) {
		Cmd_EmoteBreakdance4_f(ent);
	}
	else if (Q_stricmp(cmd, "amdance") == 0) {
		Cmd_EmoteDance_f(ent);
	}
	else if (Q_stricmp(cmd, "amhug") == 0) {
		Cmd_EmoteHug_f(ent);
	}
	else if (Q_stricmp(cmd, "amflip") == 0) {
		Cmd_EmoteSaberFlip_f(ent);
	}
	else if (Q_stricmp(cmd, "ampoint") == 0) {
		Cmd_EmotePoint_f(ent);
	}
	else if (Q_stricmp(cmd, "amsit") == 0) {
		Cmd_EmoteSit_f(ent);
	}
	else if (Q_stricmp(cmd, "amsit2") == 0) {
		Cmd_EmoteSit2_f(ent);
	}
	else if (Q_stricmp(cmd, "amsit3") == 0) {
		Cmd_EmoteSit3_f(ent);
	}
	else if (Q_stricmp(cmd, "amsit4") == 0) {
		Cmd_EmoteSit4_f(ent);
	}
	else if (Q_stricmp(cmd, "amslap") == 0) {
		Cmd_EmoteSlap_f(ent);
	}
	else if (Q_stricmp(cmd, "amsleep") == 0) {
		Cmd_EmoteSleep_f(ent);
	}
	else if (Q_stricmp(cmd, "amsurrender") == 0) {
		Cmd_EmoteSurrender_f(ent);
	}
	else if (Q_stricmp(cmd, "amsmack") == 0) {
		Cmd_EmoteSmack_f(ent);
	}
	else if (Q_stricmp(cmd, "amlogin") == 0) {
		Cmd_Amlogin_f(ent);
		return;
	}
	else if (Q_stricmp(cmd, "amlogout") == 0) {
		Cmd_Amlogout_f(ent);
		return;
	}
	else if (Q_stricmp(cmd, "amrename") == 0) {
		Cmd_Amrename_f(ent);
		return;
	}
	else if (Q_stricmp(cmd, "amtele") == 0) {
		Cmd_Amtele_f(ent);
	}
	else if (Q_stricmp(cmd, "amtelemark") == 0) {
		Cmd_Amtelemark_f(ent);
	}
	else if (Q_stricmp(cmd, "amfreeze") == 0) {
		Cmd_Amfreeze_f(ent);
	}
	else if (Q_stricmp(cmd, "amban") == 0) {
		Cmd_Amban_f(ent);
	}
	else if (Q_stricmp(cmd, "amkick") == 0) {
		Cmd_Amkick_f(ent);
	}
	else if (Q_stricmp(cmd, "amgrantadmin") == 0) {
		Cmd_Amban_f(ent);
	}
	else if (Q_stricmp(cmd, "ammap") == 0) {
		Cmd_Ammap_f(ent);
	}
	else if (Q_stricmp(cmd, "ampsay") == 0) {
		Cmd_Ampsay_f(ent);
	}
	else if (Q_stricmp(cmd, "amforceteam") == 0) {
		Cmd_Amforceteam_f(ent);
	}
	else if (Q_stricmp(cmd, "amlockteam") == 0) {
		Cmd_Amlockteam_f(ent);
	}
	else if (Q_stricmp(cmd, "amvstr") == 0) {
		Cmd_Amvstr_f(ent);
	}
	else if (Q_stricmp(cmd, "amlistmaps") == 0) {
		Cmd_AmMapList_f(ent);
	}
	else if (Q_stricmp(cmd, "amlookup") == 0) {
		Cmd_Amlookup_f(ent);
	}
	else if (Q_stricmp(cmd, "amkillvote") == 0) {
		Cmd_Amkillvote_f(ent);
	}
	else
	{
		if (Q_stricmp(cmd, "addbot") == 0)
		{ //because addbot isn't a recognized command unless you're the server, but it is in the menus regardless
//			trap_SendServerCommand( clientNum, va("print \"You can only add bots as the server.\n\"" ) );
			trap_SendServerCommand( clientNum, va("print \"%s.\n\"", G_GetStripEdString("SVINGAME", "ONLY_ADD_BOTS_AS_SERVER")));
		}
		else
		{
			trap_SendServerCommand( clientNum, va("print \"unknown cmd %s\n\"", cmd ) );
		}
	}
}
