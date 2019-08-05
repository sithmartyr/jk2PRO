// Copyright (C) 1999-2000 Id Software, Inc.
//

// this file holds commands that can be executed by the server console, but not remote clients

#include "g_local.h"


/*
==============================================================================

PACKET FILTERING
 

You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and any unspecified digits will match any value, so you can specify an entire class C network with "addip 192.246.40".

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single host.

listip
Prints the current list of filters.

g_filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.


==============================================================================
*/

// extern	vmCvar_t	g_banIPs;
// extern	vmCvar_t	g_filterBan;


typedef struct ipFilter_s
{
	unsigned	mask;
	unsigned	compare;
} ipFilter_t;

#define	MAX_IPFILTERS	1024

static ipFilter_t	ipFilters[MAX_IPFILTERS];
static int			numIPFilters;

/*
=================
StringToFilter
=================
*/
static qboolean StringToFilter (char *s, ipFilter_t *f)
{
	char		num[128];
	int			i, j;
	unsigned	compare = 0;
	unsigned	mask = 0;
	byte		*c = (byte *)&compare;
	byte		*m = (byte *)&mask;

	for (i=0 ; i<4 ; i++)
	{
		if (*s < '0' || *s > '9')
		{
			G_Printf( "Bad filter address: %s\n", s );
			return qfalse;
		}

		j = 0;
		while (*s >= '0' && *s <= '9')
		{
			num[j++] = *s++;
		}
		num[j] = 0;
		c[i] = atoi(num);
		if (c[i] != 0)
			m[i] = 255;

		if (!*s)
			break;
		s++;
	}

	f->mask = mask;
	f->compare = compare;

	return qtrue;
}

/*
=================
UpdateIPBans
=================
*/
static void UpdateIPBans (void)
{
	byte	*b;
	int		i;
	char	iplist[MAX_INFO_STRING];

	*iplist = 0;
	for (i = 0 ; i < numIPFilters ; i++)
	{
		if (ipFilters[i].compare == 0xffffffff)
			continue;

		b = (byte *)&ipFilters[i].compare;
		Com_sprintf( iplist + strlen(iplist), sizeof(iplist) - strlen(iplist), 
			"%i.%i.%i.%i ", b[0], b[1], b[2], b[3]);
	}

	trap_Cvar_Set( "g_banIPs", iplist );
}

/*
=================
G_FilterPacket
=================
*/
qboolean G_FilterPacket (char *from)
{
	int			i;
	unsigned	mask = 0;
	byte		*m = (byte *)&mask;
	char		*p;

	i = 0;
	p = from;
	while (*p && i < 4) {
		while (*p >= '0' && *p <= '9') {
			m[i] = m[i]*10 + (*p - '0');
			p++;
		}
		if (!*p || *p == ':')
			break;
		i++, p++;
	}

	for (i=0 ; i<numIPFilters ; i++)
		if ( (mask & ipFilters[i].mask) == ipFilters[i].compare)
			return g_filterBan.integer != 0;

	return g_filterBan.integer == 0;
}

/*
=================
AddIP
=================
*/
void AddIP( char *str )
{
	int		i;

	for (i = 0 ; i < numIPFilters ; i++)
		if (ipFilters[i].compare == 0xffffffffu)
			break;		// free spot
	if (i == numIPFilters)
	{
		if (numIPFilters == MAX_IPFILTERS)
		{
			G_Printf ("IP filter list is full\n");
			return;
		}
		numIPFilters++;
	}
	
	if (!StringToFilter (str, &ipFilters[i]))
		ipFilters[i].compare = 0xffffffffu;

	UpdateIPBans();
}

/*
=================
G_ProcessIPBans
=================
*/
void G_ProcessIPBans(void) 
{
	char *s, *t;
	char		str[MAX_TOKEN_CHARS];

	Q_strncpyz( str, g_banIPs.string, sizeof(str) );

	for (t = s = g_banIPs.string; *t; /* */ ) {
		s = strchr(s, ' ');
		if (!s)
			break;
		while (*s == ' ')
			*s++ = 0;
		if (*t)
			AddIP( t );
		t = s;
	}
}


/*
=================
Svcmd_AddIP_f
=================
*/
void Svcmd_AddIP_f (void)
{
	char		str[MAX_TOKEN_CHARS];

	if ( trap_Argc() < 2 ) {
		G_Printf("Usage:  addip <ip-mask>\n");
		return;
	}

	trap_Argv( 1, str, sizeof( str ) );

	AddIP( str );

}

/*
=================
Svcmd_RemoveIP_f
=================
*/
void Svcmd_RemoveIP_f (void)
{
	ipFilter_t	f;
	int			i;
	char		str[MAX_TOKEN_CHARS];

	if ( trap_Argc() < 2 ) {
		G_Printf("Usage:  sv removeip <ip-mask>\n");
		return;
	}

	trap_Argv( 1, str, sizeof( str ) );

	if (!StringToFilter (str, &f))
		return;

	for (i=0 ; i<numIPFilters ; i++) {
		if (ipFilters[i].mask == f.mask	&&
			ipFilters[i].compare == f.compare) {
			ipFilters[i].compare = 0xffffffffu;
			G_Printf ("Removed.\n");

			UpdateIPBans();
			return;
		}
	}

	G_Printf ( "Didn't find %s.\n", str );
}

/*
===================
Svcmd_EntityList_f
===================
*/
void	Svcmd_EntityList_f (void) {
	int			e;
	gentity_t		*check;

	check = g_entities+1;
	for (e = 0; e < level.num_entities ; e++, check++) {
		if ( !check->inuse ) {
			continue;
		}
		G_Printf("%3i:", e);
		switch ( check->s.eType ) {
		case ET_GENERAL:
			G_Printf("ET_GENERAL          ");
			break;
		case ET_PLAYER:
			G_Printf("ET_PLAYER           ");
			break;
		case ET_ITEM:
			G_Printf("ET_ITEM             ");
			break;
		case ET_MISSILE:
			G_Printf("ET_MISSILE          ");
			break;
		case ET_MOVER:
			G_Printf("ET_MOVER            ");
			break;
		case ET_BEAM:
			G_Printf("ET_BEAM             ");
			break;
		case ET_PORTAL:
			G_Printf("ET_PORTAL           ");
			break;
		case ET_SPEAKER:
			G_Printf("ET_SPEAKER          ");
			break;
		case ET_PUSH_TRIGGER:
			G_Printf("ET_PUSH_TRIGGER     ");
			break;
		case ET_TELEPORT_TRIGGER:
			G_Printf("ET_TELEPORT_TRIGGER ");
			break;
		case ET_INVISIBLE:
			G_Printf("ET_INVISIBLE        ");
			break;
		case ET_GRAPPLE:
			G_Printf("ET_GRAPPLE          ");
			break;
		default:
			G_Printf("%3i                 ", check->s.eType);
			break;
		}

		if ( check->classname ) {
			G_Printf("%s", check->classname);
		}
		G_Printf("\n");
	}
}

gclient_t	*ClientForString( const char *s ) {
	gclient_t	*cl;
	int			i;
	int			idnum;

	// numeric values are just slot numbers
	if ( s[0] >= '0' && s[0] <= '9' ) {
		idnum = atoi( s );
		if ( idnum < 0 || idnum >= level.maxclients ) {
			Com_Printf( "Bad client slot: %i\n", idnum );
			return NULL;
		}

		cl = &level.clients[idnum];
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			G_Printf( "Client %i is not connected\n", idnum );
			return NULL;
		}
		return cl;
	}

	// check for a name match
	for ( i=0 ; i < level.maxclients ; i++ ) {
		cl = &level.clients[i];
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( !Q_stricmp( cl->pers.netname, s ) ) {
			return cl;
		}
	}

	G_Printf( "User %s is not on the server\n", s );

	return NULL;
}

static int SV_ClientNumberFromString(const char *s)
{
	gclient_t	*cl;
	int			idnum, i, match = -1;
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9' && strlen(s) == 1) //changed this to only recognize numbers 0-31 as client numbers, otherwise interpret as a name, in which case sanitize2 it and accept partial matches (return error if multiple matches)
	{
		idnum = atoi(s);
		cl = &level.clients[idnum];
		if (cl->pers.connected != CON_CONNECTED) {
			Com_Printf("Client '%i' is not active\n", idnum);
			return -1;
		}
		return idnum;
	}

	else if (s[0] == '1' && s[0] == '2' && (s[1] >= '0' && s[1] <= '9' && strlen(s) == 2))
	{
		idnum = atoi(s);
		cl = &level.clients[idnum];
		if (cl->pers.connected != CON_CONNECTED) {
			Com_Printf("Client '%i' is not active\n", idnum);
			return -1;
		}
		return idnum;
	}

	else if (s[0] == '3' && (s[1] >= '0' && s[1] <= '1' && strlen(s) == 2))
	{
		idnum = atoi(s);
		cl = &level.clients[idnum];
		if (cl->pers.connected != CON_CONNECTED) {
			Com_Printf("Client '%i' is not active\n", idnum);
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
					Com_Printf("More than one user '%s' on the server\n", s);
					return -2;
				}
				match = level.sortedClients[i];
			}
		}
		if (match != -1)//uhh
			return match;
	}
	Com_Printf("User '%s' is not on the server\n", s);
	return -1;
}

void Svcmd_AmKick_f(void) {
	int clientid = -1;
	char   arg[MAX_NETNAME];

	if (trap_Argc() != 2) {
		trap_Printf("Usage: /amKick <client>.\n");
		return;
	}
	trap_Argv(1, arg, sizeof(arg));
	clientid = SV_ClientNumberFromString(arg);

	if (clientid == -1 || clientid == -2)
		return;
	trap_SendConsoleCommand(EXEC_APPEND, va("clientkick %i", clientid));

}

void Svcmd_AmBan_f(void) {
	int clientid = -1;
	char   arg[MAX_NETNAME];

	if (trap_Argc() != 2) {
		trap_Printf("Usage: /amBan <client>.\n");
		return;
	}
	trap_Argv(1, arg, sizeof(arg));
	clientid = SV_ClientNumberFromString(arg);

	if (clientid == -1 || clientid == -2)
		return;
	AddIP(g_entities[clientid].client->sess.IP);
	trap_SendConsoleCommand(EXEC_APPEND, va("clientkick %i", clientid));
}

void Svcmd_Amgrantadmin_f(void)
{
	char arg[MAX_NETNAME];
	int clientid = -1;

	if (trap_Argc() != 3) {
		trap_Printf("Usage: /amGrantAdmin <client> <level>.\n");
		return;
	}

	trap_Argv(1, arg, sizeof(arg));
	clientid = SV_ClientNumberFromString(arg);

	if (clientid == -1 || clientid == -2)
		return;

	if (!g_entities[clientid].client)
		return;

	trap_Argv(2, arg, sizeof(arg));
	Q_strlwr(arg);

	if (!Q_stricmp(arg, "none")) {
		int i;
		for (i = 0; i <= JAPRO_MAX_ADMIN_BITS; i++) {//Loop this 0-22 is admin flags.
			g_entities[clientid].client->sess.accountFlags &= ~(1 << i);
		}
	}
	else if (!Q_stricmp(arg, "junior")) {
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
	else if (!Q_stricmp(arg, "full")) {
		int i;
		qboolean added = qfalse;
		for (i = 0; i <= JAPRO_MAX_ADMIN_BITS; i++) {//Loop this 0-22 is admin flags.
			if (jp_fullAdminLevel.integer & (1 << i)) {
				g_entities[clientid].client->sess.accountFlags |= (1 << i);
				added = qtrue;
			}
		}
		if (added)
			trap_SendServerCommand(clientid, "print \"You have been granted Full admin privileges.\n\"");
	}
}



/*
===================
Svcmd_ForceTeam_f

forceteam <player> <team>
===================
*/
void	Svcmd_ForceTeam_f( void ) {
	gclient_t	*cl;
	char		str[MAX_TOKEN_CHARS];

	// find the player
	trap_Argv( 1, str, sizeof( str ) );
	cl = ClientForString( str );
	if ( !cl ) {
		return;
	}

	// set the team
	trap_Argv( 2, str, sizeof( str ) );
	SetTeam( &g_entities[cl - level.clients], str,  qtrue);
}

typedef struct bitInfo_S {
	const char	*string;
} bitInfo_T;

static bitInfo_T weaponTweaks[] = { // MAX_WEAPON_TWEAKS tweaks (24)
	//{ "Nonrandom DEMP2" },//1
	{ "Increased DEMP2 primary damage" },//1
	{ "Decreased disruptor alt damage" },//2
	{ "Nonrandom bowcaster spread" },//3
	{ "Increased repeater alt damage" },//4
	{ "Nonrandom flechette primary spread" },//5
	{ "Decreased flechette alt damage" },//6
	{ "Nonrandom flechette alt spread" },//7
	//{ "Increased concussion rifle alt damage" },
	{ "Removed projectile knockback" },//8
	{ "Stun baton lightning gun" },//9
	{ "Stun baton shocklance" },//10
	{ "Projectile gravity" },//11
	{ "Allow center muzzle" },//12
	{ "Pseudo random weapon spread" },//13
	//{ "Rocket alt fire mortar" },//14
	//{ "Rocket alt fire redeemer" },//15
	{ "Infinite ammo" },//14
	{ "Stun baton heal gun" },//15
	//{ "Weapons can damage vehicles" },
	//{ "Allow gunroll" },
	{ "Fast weaponswitch" },//16
	{ "Impact nitrons" },//17
	{ "Flechette stake gun" },//18
	{ "Fix dropped mine ammo count" },//19
	//{ "JK2 Style Alt Tripmine" },
	{ "Projectile Sniper" },//20
	{ "No Spread" },//21
	{ "Slow sniper fire rate" },//22
	{ "Make rockets solid for their owners" },//23
	{ "Lower max damage for pistol alt fire" }//24
};
static const int MAX_WEAPON_TWEAKS = ARRAY_LEN(weaponTweaks);

void CVU_TweakWeapons(void) {
	(jp_tweakWeapons.integer & WT_PSEUDORANDOM_FIRE) ?
		(jcinfo.integer |= JAPRO_CINFO_PSEUDORANDOM_FIRE) : (jcinfo.integer &= ~JAPRO_CINFO_PSEUDORANDOM_FIRE);
	(jp_tweakWeapons.integer & WT_STUN_LG) ?
		(jcinfo.integer |= JAPRO_CINFO_LG) : (jcinfo.integer &= ~JAPRO_CINFO_LG);
	(jp_tweakWeapons.integer & WT_STUN_SHOCKLANCE) ?
		(jcinfo.integer |= JAPRO_CINFO_SHOCKLANCE) : (jcinfo.integer &= ~JAPRO_CINFO_SHOCKLANCE);
	/*(jp_tweakWeapons.integer & WT_ALLOW_GUNROLL) ?
		(jcinfo.integer |= JAPRO_CINFO_GUNROLL) : (jcinfo.integer &= ~JAPRO_CINFO_GUNROLL);*/
	(jp_tweakWeapons.integer & WT_PROJ_SNIPER) ?
		(jcinfo.integer |= JAPRO_CINFO_PROJSNIPER) : (jcinfo.integer &= ~JAPRO_CINFO_PROJSNIPER);
	trap_Cvar_Set("jcinfo", va("%i", jcinfo.integer));
}

void GiveClientWeapons(gclient_t *client);

static void RemoveWeaponsFromPlayer(gentity_t *ent) {
	int disallowedWeaps = g_weaponDisable.integer & ~jp_startingWeapons.integer;

	ent->client->ps.stats[STAT_WEAPONS] &= ~disallowedWeaps; //Subtract disallowed weapons from current weapons.

	if (ent->client->ps.stats[STAT_WEAPONS] <= 0)
		ent->client->ps.stats[STAT_WEAPONS] = WP_NONE;

	if (!(ent->client->ps.stats[STAT_WEAPONS] & (1 >> ent->client->ps.weapon))) { //If our weapon selected does not appear in our weapons list
		ent->client->ps.weapon = WP_NONE; //who knows why this does shit even if our current weapon is fine.
	}
}

void CVU_StartingWeapons(void) {
	int i;
	gentity_t *ent;

	for (i = 0; i < level.numConnectedClients; i++) { //For each player, call removeweapons, and addweapons.
		ent = &g_entities[level.sortedClients[i]];
		if (ent->inuse && ent->client && !ent->client->sess.raceMode) {
			RemoveWeaponsFromPlayer(ent);
			GiveClientWeapons(ent->client);
		}
	}
}

void Svcmd_ToggleTweakWeapons_f(void) {
	if (trap_Argc() == 1) {
		int i = 0;
		for (i = 0; i < MAX_WEAPON_TWEAKS; i++) {
			if ((jp_tweakWeapons.integer & (1 << i))) {
				Com_Printf("%2d [X] %s\n", i, weaponTweaks[i].string);
			}
			else {
				Com_Printf("%2d [ ] %s\n", i, weaponTweaks[i].string);
			}
		}
		return;
	}
	else {
		char arg[8] = { 0 };
		int index;
		const uint32_t mask = (1 << MAX_WEAPON_TWEAKS) - 1;

		trap_Argv(1, arg, sizeof(arg));
		index = atoi(arg);

		if (index < 0 || index >= MAX_WEAPON_TWEAKS) {
			Com_Printf("tweakWeapons: Invalid range: %i [0, %i]\n", index, MAX_WEAPON_TWEAKS - 1);
			return;
		}

		trap_Cvar_Set("jp_tweakWeapons", va("%i", (1 << index) ^ (jp_tweakWeapons.integer & mask)));
		trap_Cvar_Update(&jp_tweakWeapons);

		Com_Printf("%s %s^7\n", weaponTweaks[index].string, ((jp_tweakWeapons.integer & (1 << index))
			? "^2Enabled" : "^1Disabled"));

		CVU_TweakWeapons();
	}
}

static bitInfo_T adminOptions[] = {
	{ "Amtele" },//0
	{ "Amfreeze" },//1
	{ "Amtelemark" },//2
	{ "Amban" },//3
	{ "Amkick" },//4
	/*{ "NPC" },//5*/
	{ "Noclip" },//5
	{ "Grantadmin" },//6
	{ "Ammap" },//7
	{ "Ampsay" },//8
	{ "Amforceteam" },//9
	{ "Amlockteam" },//10
	{ "Amvstr" },//11
	{ "See IPs" },//12
	{ "Amrename" },//13
	{ "Amlistmaps" },//14
	{ "Amwhois" },//15
	{ "Amlookup" },//16
	{ "Use hide" },//17
	{ "See hiders" },//18
	{ "Callvote" },//19
	{ "Killvote" },//20
	{ "Read Amsay" }//21
};
static const int MAX_ADMIN_OPTIONS = ARRAY_LEN(adminOptions);

void Svcmd_ToggleAdmin_f(void) {
	if (trap_Argc() == 1) {
		trap_Printf("Usage: toggleAdmin <admin level (full or junior)> <admin option>\n");
		return;
	}
	else if (trap_Argc() == 2) {
		int i, level;
		char arg1[8] = { 0 };

		trap_Argv(1, arg1, sizeof(arg1));
		if (!Q_stricmp(arg1, "j") || !Q_stricmp(arg1, "junior"))
			level = 0;
		else if (!Q_stricmp(arg1, "f") || !Q_stricmp(arg1, "full"))
			level = 1;
		else {
			Com_Printf("Usage: toggleAdmin <admin level (full or junior)> <admin option>\n");
			return;
		}

		for (i = 0; i < MAX_ADMIN_OPTIONS; i++) {
			if (level == 0) {
				if ((jp_juniorAdminLevel.integer & (1 << i))) {
					Com_Printf("%2d [X] %s\n", i, adminOptions[i].string);
				}
				else {
					Com_Printf("%2d [ ] %s\n", i, adminOptions[i].string);
				}
			}
			else if (level == 1) {
				if ((jp_fullAdminLevel.integer & (1 << i))) {
					Com_Printf("%2d [X] %s\n", i, adminOptions[i].string);
				}
				else {
					Com_Printf("%2d [ ] %s\n", i, adminOptions[i].string);
				}
			}
		}
		return;
	}
	else {
		char arg1[8] = { 0 }, arg2[8] = { 0 };
		int index, level;
		const uint32_t mask = (1 << MAX_ADMIN_OPTIONS) - 1;

		trap_Argv(1, arg1, sizeof(arg1));
		if (!Q_stricmp(arg1, "j") || !Q_stricmp(arg1, "junior"))
			level = 0;
		else if (!Q_stricmp(arg1, "f") || !Q_stricmp(arg1, "full"))
			level = 1;
		else {
			Com_Printf("Usage: toggleAdmin <admin level (full or junior)> <admin option>\n");
			return;
		}
		trap_Argv(2, arg2, sizeof(arg2));
		index = atoi(arg2);

		if (index < 0 || index >= MAX_ADMIN_OPTIONS) {
			Com_Printf("toggleAdmin: Invalid range: %i [0, %i]\n", index, MAX_ADMIN_OPTIONS - 1);
			return;
		}

		if (level == 0) {
			trap_Cvar_Set("jp_juniorAdminLevel", va("%i", (1 << index) ^ (jp_juniorAdminLevel.integer & mask)));
			trap_Cvar_Update(&jp_juniorAdminLevel);

			Com_Printf("%s %s^7\n", adminOptions[index].string, ((jp_juniorAdminLevel.integer & (1 << index)) ? "^2Enabled" : "^1Disabled"));
		}
		else if (level == 1) {
			trap_Cvar_Set("jp_fullAdminLevel", va("%i", (1 << index) ^ (jp_fullAdminLevel.integer & mask)));
			trap_Cvar_Update(&jp_fullAdminLevel);

			Com_Printf("%s %s^7\n", adminOptions[index].string, ((jp_fullAdminLevel.integer & (1 << index)) ? "^2Enabled" : "^1Disabled"));
		}
	}
}

char	*ConcatArgs( int start );

/*
=================
ConsoleCommand

=================
*/
qboolean	ConsoleCommand( void ) {
	char	cmd[MAX_TOKEN_CHARS];

	trap_Argv( 0, cmd, sizeof( cmd ) );

	if ( Q_stricmp (cmd, "entitylist") == 0 ) {
		Svcmd_EntityList_f();
		return qtrue;
	}

	if (Q_stricmp(cmd, "amkick") == 0) {
		Svcmd_AmKick_f();
		return qtrue;
	}

	if (Q_stricmp(cmd, "amban") == 0) {
		Svcmd_AmBan_f();
		return qtrue;
	}

	if (Q_stricmp(cmd, "amgrantadmin") == 0) {
		Svcmd_Amgrantadmin_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "forceteam") == 0 ) {
		Svcmd_ForceTeam_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "game_memory") == 0) {
		Svcmd_GameMem_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "addbot") == 0) {
		Svcmd_AddBot_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "botlist") == 0) {
		Svcmd_BotList_f();
		return qtrue;
	}

/*	if (Q_stricmp (cmd, "abort_podium") == 0) {
		Svcmd_AbortPodium_f();
		return qtrue;
	}
*/
	if (Q_stricmp (cmd, "addip") == 0) {
		Svcmd_AddIP_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "removeip") == 0) {
		Svcmd_RemoveIP_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "listip") == 0) {
		trap_SendConsoleCommand( EXEC_NOW, "g_banIPs\n" );
		return qtrue;
	}

	if (Q_stricmp(cmd, "toggleadmin") == 0) {
		Svcmd_ToggleAdmin_f();
		return qtrue;
	}

	if (Q_stricmp(cmd, "tweakweapons") == 0) {
		Svcmd_ToggleTweakWeapons_f();
		return qtrue;
	}

#if _DEBUG // Only in debug builds
	if ( !Q_stricmp(cmd, "jk2gameplay") )
	{
		char arg1[MAX_TOKEN_CHARS];

		trap_Argv( 1, arg1, sizeof(arg1) );

		switch ( atoi(arg1) )
		{
			case VERSION_1_02:
				MV_SetGamePlay(VERSION_1_02);
				trap_SendServerCommand( -1, "print \"Gameplay changed to 1.02\n\"" );
				break;
			case VERSION_1_03:
				MV_SetGamePlay(VERSION_1_03);
				trap_SendServerCommand( -1, "print \"Gameplay changed to 1.03\n\"" );
				break;
			default:
			case VERSION_1_04:
				MV_SetGamePlay(VERSION_1_04);
				trap_SendServerCommand( -1, "print \"Gameplay changed to 1.04\n\"" );
				break;
		}
		return qtrue;
	}
#endif

	if (g_dedicated.integer) {
		if (Q_stricmp (cmd, "say") == 0) {
			trap_SendServerCommand( -1, va("print \"server: %s\n\"", ConcatArgs(1) ) );
			return qtrue;
		}
		// everything else will also be printed as a say command
		trap_SendServerCommand( -1, va("print \"server: %s\n\"", ConcatArgs(0) ) );
		return qtrue;
	}

	return qfalse;
}

