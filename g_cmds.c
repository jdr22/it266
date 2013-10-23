#include "g_local.h"
#include "m_player.h"


char *ClientTeam (edict_t *ent)
{
	char		*p;
	static char	value[512];

	value[0] = 0;

	if (!ent->client)
		return value;

	strcpy(value, Info_ValueForKey (ent->client->pers.userinfo, "skin"));
	p = strchr(value, '/');
	if (!p)
		return value;

	if ((int)(dmflags->value) & DF_MODELTEAMS)
	{
		*p = 0;
		return value;
	}

	// if ((int)(dmflags->value) & DF_SKINTEAMS)
	return ++p;
}

qboolean OnSameTeam (edict_t *ent1, edict_t *ent2)
{
	char	ent1Team [512];
	char	ent2Team [512];

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		return false;

	strcpy (ent1Team, ClientTeam (ent1));
	strcpy (ent2Team, ClientTeam (ent2));

	if (strcmp(ent1Team, ent2Team) == 0)
		return true;
	return false;
}


void SelectNextItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;

	cl = ent->client;

	if (cl->chase_target) {
		ChaseNext(ent);
		return;
	}

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (cl->pers.selected_item + i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void SelectPrevItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;

	cl = ent->client;

	if (cl->chase_target) {
		ChasePrev(ent);
		return;
	}

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (cl->pers.selected_item + MAX_ITEMS - i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void ValidateSelectedItem (edict_t *ent)
{
	gclient_t	*cl;

	cl = ent->client;

	if (cl->pers.inventory[cl->pers.selected_item])
		return;		// valid

	SelectNextItem (ent, -1);
}


//=================================================================================

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f (edict_t *ent)
{
	char		*name;
	gitem_t		*it;
	int			index;
	int			i;
	qboolean	give_all;
	edict_t		*it_ent;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	name = gi.args();

	if (Q_stricmp(name, "all") == 0)
		give_all = true;
	else
		give_all = false;

	if (give_all || Q_stricmp(gi.argv(1), "health") == 0)
	{
		if (gi.argc() == 3)
			ent->health = atoi(gi.argv(2));
		else
			ent->health = ent->max_health;
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_WEAPON))
				continue;
			ent->client->pers.inventory[i] += 1;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_AMMO))
				continue;
			Add_Ammo (ent, it, 1000);
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		gitem_armor_t	*info;

		it = FindItem("Jacket Armor");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Combat Armor");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Body Armor");
		info = (gitem_armor_t *)it->info;
		ent->client->pers.inventory[ITEM_INDEX(it)] = info->max_count;

		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "Power Shield") == 0)
	{
		it = FindItem("Power Shield");
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);

		if (!give_all)
			return;
	}

	if (give_all)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (it->flags & (IT_ARMOR|IT_WEAPON|IT_AMMO))
				continue;
			ent->client->pers.inventory[i] = 1;
		}
		return;
	}

	it = FindItem (name);
	if (!it)
	{
		name = gi.argv(1);
		it = FindItem (name);
		if (!it)
		{
			gi.cprintf (ent, PRINT_HIGH, "unknown item\n");
			return;
		}
	}

	if (!it->pickup)
	{
		gi.cprintf (ent, PRINT_HIGH, "non-pickup item\n");
		return;
	}

	index = ITEM_INDEX(it);

	if (it->flags & IT_AMMO)
	{
		if (gi.argc() == 3)
			ent->client->pers.inventory[index] = atoi(gi.argv(2));
		else
			ent->client->pers.inventory[index] += it->quantity;
	}
	else
	{
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	if (ent->movetype == MOVETYPE_NOCLIP)
	{
		ent->movetype = MOVETYPE_WALK;
		msg = "noclip OFF\n";
	}
	else
	{
		ent->movetype = MOVETYPE_NOCLIP;
		msg = "noclip ON\n";
	}

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Use_f

Use an inventory item
==================
*/
void Cmd_Use_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->use (ent, it);
}


/*
==================
Cmd_Drop_f

Drop an inventory item
==================
*/
void Cmd_Drop_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->drop (ent, it);
}


/*
=================
Cmd_Inven_f
=================
*/
void Cmd_Inven_f (edict_t *ent)
{
	int			i;
	gclient_t	*cl;

	cl = ent->client;

	cl->showscores = false;
	cl->showhelp = false;

	if (cl->showinventory)
	{
		cl->showinventory = false;
		return;
	}

	cl->showinventory = true;

	gi.WriteByte (svc_inventory);
	for (i=0 ; i<MAX_ITEMS ; i++)
	{
		gi.WriteShort (cl->pers.inventory[i]);
	}
	gi.unicast (ent, true);
}

/*
=================
Cmd_InvUse_f
=================
*/
void Cmd_InvUse_f (edict_t *ent)
{
	gitem_t		*it;

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to use.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	it->use (ent, it);
}

/*
=================
Cmd_WeapPrev_f
=================
*/
void Cmd_WeapPrev_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (selected_weapon + i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapNext_f
=================
*/
void Cmd_WeapNext_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (selected_weapon + MAX_ITEMS - i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapLast_f
=================
*/
void Cmd_WeapLast_f (edict_t *ent)
{
	gclient_t	*cl;
	int			index;
	gitem_t		*it;

	cl = ent->client;

	if (!cl->pers.weapon || !cl->pers.lastweapon)
		return;

	index = ITEM_INDEX(cl->pers.lastweapon);
	if (!cl->pers.inventory[index])
		return;
	it = &itemlist[index];
	if (!it->use)
		return;
	if (! (it->flags & IT_WEAPON) )
		return;
	it->use (ent, it);
}

/*
=================
Cmd_InvDrop_f
=================
*/
void Cmd_InvDrop_f (edict_t *ent)
{
	gitem_t		*it;

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to drop.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	it->drop (ent, it);
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f (edict_t *ent)
{
	if((level.time - ent->client->respawn_time) < 5)
		return;
	ent->flags &= ~FL_GODMODE;
	ent->health = 0;
	meansOfDeath = MOD_SUICIDE;
	player_die (ent, ent, ent, 100000, vec3_origin);
}

/*
=================
Cmd_PutAway_f
=================
*/
void Cmd_PutAway_f (edict_t *ent)
{
	ent->client->showscores = false;
	ent->client->showhelp = false;
	ent->client->showinventory = false;
}


int PlayerSort (void const *a, void const *b)
{
	int		anum, bnum;

	anum = *(int *)a;
	bnum = *(int *)b;

	anum = game.clients[anum].ps.stats[STAT_FRAGS];
	bnum = game.clients[bnum].ps.stats[STAT_FRAGS];

	if (anum < bnum)
		return -1;
	if (anum > bnum)
		return 1;
	return 0;
}

/*
=================
Cmd_Players_f
=================
*/
void Cmd_Players_f (edict_t *ent)
{
	int		i;
	int		count;
	char	small[64];
	char	large[1280];
	int		index[256];

	count = 0;
	for (i = 0 ; i < maxclients->value ; i++)
		if (game.clients[i].pers.connected)
		{
			index[count] = i;
			count++;
		}

	// sort by frags
	qsort (index, count, sizeof(index[0]), PlayerSort);

	// print information
	large[0] = 0;

	for (i = 0 ; i < count ; i++)
	{
		Com_sprintf (small, sizeof(small), "%3i %s\n",
			game.clients[index[i]].ps.stats[STAT_FRAGS],
			game.clients[index[i]].pers.netname);
		if (strlen (small) + strlen(large) > sizeof(large) - 100 )
		{	// can't print all of them in one packet
			strcat (large, "...\n");
			break;
		}
		strcat (large, small);
	}

	gi.cprintf (ent, PRINT_HIGH, "%s\n%i players\n", large, count);
}

/*
=================
Cmd_Wave_f
=================
*/
void Cmd_Wave_f (edict_t *ent)
{
	int		i;

	i = atoi (gi.argv(1));

	// can't wave when ducked
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		return;

	if (ent->client->anim_priority > ANIM_WAVE)
		return;

	ent->client->anim_priority = ANIM_WAVE;

	switch (i)
	{
	case 0:
		gi.cprintf (ent, PRINT_HIGH, "flipoff\n");
		ent->s.frame = FRAME_flip01-1;
		ent->client->anim_end = FRAME_flip12;
		break;
	case 1:
		gi.cprintf (ent, PRINT_HIGH, "salute\n");
		ent->s.frame = FRAME_salute01-1;
		ent->client->anim_end = FRAME_salute11;
		break;
	case 2:
		gi.cprintf (ent, PRINT_HIGH, "taunt\n");
		ent->s.frame = FRAME_taunt01-1;
		ent->client->anim_end = FRAME_taunt17;
		break;
	case 3:
		gi.cprintf (ent, PRINT_HIGH, "wave\n");
		ent->s.frame = FRAME_wave01-1;
		ent->client->anim_end = FRAME_wave11;
		break;
	case 4:
	default:
		gi.cprintf (ent, PRINT_HIGH, "point\n");
		ent->s.frame = FRAME_point01-1;
		ent->client->anim_end = FRAME_point12;
		break;
	}
}

/*
==================
Cmd_Say_f
==================
*/
void Cmd_Say_f (edict_t *ent, qboolean team, qboolean arg0)
{
	int		i, j;
	edict_t	*other;
	char	*p;
	char	text[2048];
	gclient_t *cl;

	if (gi.argc () < 2 && !arg0)
		return;

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		team = false;

	if (team)
		Com_sprintf (text, sizeof(text), "(%s): ", ent->client->pers.netname);
	else
		Com_sprintf (text, sizeof(text), "%s: ", ent->client->pers.netname);

	if (arg0)
	{
		strcat (text, gi.argv(0));
		strcat (text, " ");
		strcat (text, gi.args());
	}
	else
	{
		p = gi.args();

		if (*p == '"')
		{
			p++;
			p[strlen(p)-1] = 0;
		}
		strcat(text, p);
	}

	// don't let text be too long for malicious reasons
	if (strlen(text) > 150)
		text[150] = 0;

	strcat(text, "\n");

	if (flood_msgs->value) {
		cl = ent->client;

        if (level.time < cl->flood_locktill) {
			gi.cprintf(ent, PRINT_HIGH, "You can't talk for %d more seconds\n",
				(int)(cl->flood_locktill - level.time));
            return;
        }
        i = cl->flood_whenhead - flood_msgs->value + 1;
        if (i < 0)
            i = (sizeof(cl->flood_when)/sizeof(cl->flood_when[0])) + i;
		if (cl->flood_when[i] && 
			level.time - cl->flood_when[i] < flood_persecond->value) {
			cl->flood_locktill = level.time + flood_waitdelay->value;
			gi.cprintf(ent, PRINT_CHAT, "Flood protection:  You can't talk for %d seconds.\n",
				(int)flood_waitdelay->value);
            return;
        }
		cl->flood_whenhead = (cl->flood_whenhead + 1) %
			(sizeof(cl->flood_when)/sizeof(cl->flood_when[0]));
		cl->flood_when[cl->flood_whenhead] = level.time;
	}

	if (dedicated->value)
		gi.cprintf(NULL, PRINT_CHAT, "%s", text);

	for (j = 1; j <= game.maxclients; j++)
	{
		other = &g_edicts[j];
		if (!other->inuse)
			continue;
		if (!other->client)
			continue;
		if (team)
		{
			if (!OnSameTeam(ent, other))
				continue;
		}
		gi.cprintf(other, PRINT_CHAT, "%s", text);
	}
}

void Cmd_PlayerList_f(edict_t *ent)
{
	int i;
	char st[80];
	char text[1400];
	edict_t *e2;

	// connect time, ping, score, name
	*text = 0;
	for (i = 0, e2 = g_edicts + 1; i < maxclients->value; i++, e2++) {
		if (!e2->inuse)
			continue;

		Com_sprintf(st, sizeof(st), "%02d:%02d %4d %3d %s%s\n",
			(level.framenum - e2->client->resp.enterframe) / 600,
			((level.framenum - e2->client->resp.enterframe) % 600)/10,
			e2->client->ping,
			e2->client->resp.score,
			e2->client->pers.netname,
			e2->client->resp.spectator ? " (spectator)" : "");
		if (strlen(text) + strlen(st) > sizeof(text) - 50) {
			sprintf(text+strlen(text), "And more...\n");
			gi.cprintf(ent, PRINT_HIGH, "%s", text);
			return;
		}
		strcat(text, st);
	}
	gi.cprintf(ent, PRINT_HIGH, "%s", text);
}

void Cmd_SetClass(edict_t *ent)
{
	int		i;
	gclient_t *client;

	i = atoi (gi.argv(1));
	client = ent->client;

	if(client->pers.playerClass == 1 || client->pers.playerClass == 2 || client->pers.playerClass == 3)
	{
		gi.cprintf (ent, PRINT_CHAT, "You already chose a class. Kill yourself if you want to pick a new class.\n");
		return;
	}

	if(i == CLASS_SUPPORT )
	{
		gi.cprintf (ent, PRINT_CHAT, "You are now support class\n");	
		client->pers.playerClass = 1; // set current
		client->pers.tierOneUpgradeLevel = 0; // no upgrades yet
		client->pers.max_shells = 999;
		ent->max_health = 125;
		ent->health = ent->max_health;
	}
	else
	if(i == CLASS_HEAVY )
	{
		gi.cprintf (ent, PRINT_CHAT, "You are now heavy class\n"); 
		client->pers.playerClass = 2;
		client->pers.tierOneUpgradeLevel = 0;
		client->pers.max_bullets = 999;
		ent->max_health = 300;
		ent->health = ent->max_health;
	}
	else
	if(i == CLASS_DEMO )
	{
		gi.cprintf (ent, PRINT_CHAT, "You are now demo class\n");
		client->pers.playerClass = 3;
		client->pers.tierOneUpgradeLevel = 0;
		client->pers.max_grenades = 999;
		client->pers.max_rockets = 999;
		ent->max_health = 200;
		ent->health = ent->max_health;
	} 
	else
	{
		gi.cprintf (ent, PRINT_CHAT, "Not a valid class choice\n");
	}

}

void Cmd_PrintClass(edict_t *ent)
{
	gclient_t *client;
	int i;

	client = ent->client;
	i = client->pers.playerClass;

	if(i == CLASS_SUPPORT )
	{
		gi.cprintf (ent, PRINT_HIGH, "You are support class\n");
		
	}
	else
	if(i == CLASS_HEAVY )
	{
		gi.cprintf (ent, PRINT_HIGH, "You are heavy class\n");
	}
	else
	if(i == CLASS_DEMO )
	{
		gi.cprintf (ent, PRINT_HIGH, "You are demo class\n");
	} 
	else
	{
		gi.cprintf (ent, PRINT_HIGH, "No Class\n");
	}
}

void Cmd_BuyShotgun(edict_t *ent)
{
	gitem_t		*item;
	gitem_t		*ammo;
	int			ammoIndex;
	gclient_t *client;

	client = ent->client;
	item = FindItem("Shotgun"); // FindItem is basically a getter method
	if( client->pers.inventory[ITEM_INDEX(item)] >= 0) // since idk what the array is initialized to
	{
		client->pers.inventory[ITEM_INDEX(item)] += 1; // inventory array's values are how many of each item you have
	}
	else
		client->pers.inventory[ITEM_INDEX(item)] = 1;

	// to auto switch the weapon you do something like
	// but its buggy so i am not going to use it
	//client->pers.selected_item = ITEM_INDEX(item); // not sure what this does
	//client->pers.weapon = item; // this switches to the weapon in game

	// give em some ammo with their purchase
	ammo = FindItem("Shells");
	ammoIndex = ITEM_INDEX(ammo);
	if( client->pers.inventory[ammoIndex] >= 0) // since idk what the array is initialized to
	{
		client->pers.inventory[ammoIndex] = client->pers.inventory[ammoIndex] + 6; // this should give them +6 shotshells
	}
	else
		client->pers.inventory[ammoIndex] = 5; // this should give them 5 shotshells

	gi.cprintf (ent, PRINT_CHAT, "You just bought a Shotgun.\n");

}

void Cmd_BuySuperShotgun(edict_t *ent)
{
	gitem_t		*item;
	gitem_t		*ammo;
	int			ammoIndex;
	gclient_t *client;

	client = ent->client;
	item = FindItem("Super Shotgun"); // FindItem is basically a getter method
	if( client->pers.inventory[ITEM_INDEX(item)] >= 0) // since idk what the array is initialized to
	{
		client->pers.inventory[ITEM_INDEX(item)] += 1; // inventory array's values are how many of each item you have
	}
	else
		client->pers.inventory[ITEM_INDEX(item)] = 1;

	// give em some ammo with their purchase
	ammo = FindItem("Shells");
	ammoIndex = ITEM_INDEX(ammo);
	if( client->pers.inventory[ammoIndex] >= 0) // since idk what the array is initialized to
	{
		client->pers.inventory[ammoIndex] = client->pers.inventory[ammoIndex] + 6; // this should give them +6 shotshells
	}
	else
		client->pers.inventory[ammoIndex] = 5; // this should give them 5 shotshells

	gi.cprintf (ent, PRINT_CHAT, "You just bought a Super Shotgun.\n");

}

void Cmd_BuyMachinegun(edict_t *ent)
{
	gitem_t		*weapon;
	gitem_t		*ammo;
	int			ammoIndex;
	gclient_t *client;

	client = ent->client;
	weapon = FindItem("Machinegun"); // FindItem is basically a getter method
	if( client->pers.inventory[ITEM_INDEX(weapon)] >= 0) // since idk what the array is initialized to
	{
		client->pers.inventory[ITEM_INDEX(weapon)] += 1; // inventory array's values are how many of each item you have
	}
	else
		client->pers.inventory[ITEM_INDEX(weapon)] = 1;

	// give em some ammo with their purchase
	ammo = FindItem("Bullets");
	ammoIndex = ITEM_INDEX(ammo);
	if( client->pers.inventory[ammoIndex] >= 0) // since idk what the array is initialized to
	{
		client->pers.inventory[ammoIndex] += 30; // this should give them +30 rounds
	}
	else
		client->pers.inventory[ammoIndex] = 5; // this should give them 5 rounds

	gi.cprintf (ent, PRINT_CHAT, "You just bought a Machinegun.\n");

}

void Cmd_BuyChaingun(edict_t *ent)
{
	gitem_t		*weapon;
	int			 weaponIndex;
	gitem_t		*ammo;
	int			ammoIndex;
	gclient_t *client;

	client = ent->client;
	weapon = FindItem("Chaingun"); // FindItem is basically a getter method
	weaponIndex = ITEM_INDEX(weapon);
	if( client->pers.inventory[weaponIndex] >= 0) // since idk what the array is initialized to
	{
		client->pers.inventory[weaponIndex] += 1; // inventory array's values are how many of each item you have
	}
	else
		client->pers.inventory[weaponIndex] = 1;

	// give em some ammo with their purchase
	ammo = FindItem("Bullets");
	ammoIndex = ITEM_INDEX(ammo);
	if( client->pers.inventory[ammoIndex] >= 0) // since idk what the array is initialized to
	{
		client->pers.inventory[ammoIndex] += 30; // this should give them +30 rounds
	}
	else
		client->pers.inventory[ammoIndex] = 5; // this should give them 5 rounds

	gi.cprintf (ent, PRINT_CHAT, "You just bought a Chaingun.\n");

}

void Cmd_BuyGrenadeLauncher(edict_t *ent)
{
	gitem_t		*weapon;
	int			 weaponIndex;
	gitem_t		*ammo;
	int			ammoIndex;
	gclient_t *client;

	client = ent->client;
	weapon = FindItem("Grenade Launcher"); 
	weaponIndex = ITEM_INDEX(weapon);
	if( client->pers.inventory[weaponIndex] >= 0) // since idk what the array is initialized to
	{
		client->pers.inventory[weaponIndex] += 1; 
	}
	else
		client->pers.inventory[weaponIndex] = 1;

	// give em some ammo with their purchase
	ammo = FindItem("Grenades");
	ammoIndex = ITEM_INDEX(ammo);
	if( client->pers.inventory[ammoIndex] >= 0) // since idk what the array is initialized to
	{
		client->pers.inventory[ammoIndex] += 3;
	}
	else
		client->pers.inventory[ammoIndex] = 3; 

	gi.cprintf (ent, PRINT_CHAT, "You just bought a Grenade Launcher.\n");
}

void Cmd_BuyRocketLauncher(edict_t *ent)
{
	gitem_t		*weapon;
	int			 weaponIndex;
	gitem_t		*ammo;
	int			ammoIndex;
	gclient_t *client;

	client = ent->client;
	weapon = FindItem("Rocket Launcher"); 
	weaponIndex = ITEM_INDEX(weapon);
	if( client->pers.inventory[weaponIndex] >= 0) // since idk what the array is initialized to
	{
		client->pers.inventory[weaponIndex] += 1; 
	}
	else
		client->pers.inventory[weaponIndex] = 1;

	// give em some ammo with their purchase
	ammo = FindItem("Rockets");
	ammoIndex = ITEM_INDEX(ammo);
	if( client->pers.inventory[ammoIndex] >= 0) // since idk what the array is initialized to
	{
		client->pers.inventory[ammoIndex] += 3;
	}
	else
		client->pers.inventory[ammoIndex] = 3; 

	gi.cprintf (ent, PRINT_CHAT, "You just bought a Rocket Launcher.\n");
}

void Cmd_BuyTierOne(edict_t *ent)
{
	gitem_t	*weapon;
	int shotgunIndex;
	int machinegunIndex;
	int grenadeLauncherIndex;

	weapon = FindItem("Shotgun");
	shotgunIndex = ITEM_INDEX(weapon);
	weapon = FindItem("Machinegun");
	machinegunIndex = ITEM_INDEX(weapon);
	weapon = FindItem("Grenade Launcher"); 
	grenadeLauncherIndex = ITEM_INDEX(weapon);
	

	// if our class is support
	if(ent->client->pers.playerClass == CLASS_SUPPORT)
	{
		// if we have a shotgun
		if( ent->client->pers.inventory[shotgunIndex] > 0 )
		{
			// if we have a 0 lvl gun
			if(ent->client->pers.tierOneUpgradeLevel == 0)
			{
				// perform the lvl 1 upgrade
				if(ent->client->pers.cash >= 200)
				{
					ent->client->pers.cash -= 200;
					ent->client->pers.tierOneUpgradeLevel = 1;
					gi.cprintf (ent, PRINT_CHAT, "You upgraded your Shotgun to LVL 1.\n");
				}
				else
					gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
			}
			else // we have a lvl 1 gun
			if(ent->client->pers.tierOneUpgradeLevel == 1)
			{
				// upgrade to 2
				if(ent->client->pers.cash >= 300)
				{
					ent->client->pers.cash -= 300;
					ent->client->pers.tierOneUpgradeLevel = 2;
					gi.cprintf (ent, PRINT_CHAT, "You upgraded your Shotgun to LVL 2.\n");
				}
				else
					gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
			}
			else // we have a lvl 2 gun
			if(ent->client->pers.tierOneUpgradeLevel == 2)
			{
				//upgrade to 3
				if(ent->client->pers.cash >= 400)
				{
					ent->client->pers.cash -= 400;
					ent->client->pers.tierOneUpgradeLevel = 3;
					gi.cprintf (ent, PRINT_CHAT, "You upgraded your Shotgun to LVL 3.\n");
				}
				else
					gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
			}
			else // we have a lvl 3 or higher gun
			if(ent->client->pers.tierOneUpgradeLevel >= 3)
			{
				gi.cprintf (ent, PRINT_CHAT, "Your Shotgun upgrades are maxed out!\n");
			}
			else // our gun lvl got messed up and is undefined
			{
				gi.cprintf (ent, PRINT_CHAT, "That's weird. Your Shotgun upgrade level is undefined.\n");
			}

		}
		else // we are support class and dont have a shotgun
		{
			if(ent->client->pers.cash >= 100)
			{
				ent->client->pers.cash -= 100;
				Cmd_BuyShotgun(ent);
			}
			else
				gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
		}
					
	}
	else // if we are heavy class
	if( ent->client->pers.playerClass == CLASS_HEAVY)
	{
		// if we have a machinegun
		if(ent->client->pers.inventory[machinegunIndex] > 0 )
		{
				// if we have a 0 lvl gun
				if(ent->client->pers.tierOneUpgradeLevel == 0)
				{
					// upgrade to 1
					if(ent->client->pers.cash >= 200)
					{
						ent->client->pers.cash -= 200;
						ent->client->pers.tierOneUpgradeLevel = 1;
						gi.cprintf (ent, PRINT_CHAT, "You upgraded your Machinegun to LVL 1.\n");
					}
					else
						gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
				}
				else // if we have lvl 1 gun
				if(ent->client->pers.tierOneUpgradeLevel == 1)
				{
					// upgrade to 2
					if(ent->client->pers.cash >= 300)
					{
						ent->client->pers.cash -= 300;
						ent->client->pers.tierOneUpgradeLevel = 2;
						gi.cprintf (ent, PRINT_CHAT, "You upgraded your Machinegun to LVL 2.\n");

					}
					else
						gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
				}
				else // if we have a lvl 2 gun
				if(ent->client->pers.tierOneUpgradeLevel == 2)
				{
					//upgrade to 3
					if(ent->client->pers.cash >= 400)
					{
						ent->client->pers.cash -= 400;
						ent->client->pers.tierOneUpgradeLevel = 3;
						gi.cprintf (ent, PRINT_CHAT, "You upgraded your Machinegun to LVL 3.\n");
					}
					else
						gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
				}
				else // we have a lvl 3 or higher gun
				if(ent->client->pers.tierOneUpgradeLevel >= 3)
				{
					gi.cprintf (ent, PRINT_CHAT, "Your Machinegun upgrades are maxed out!\n");
				}
				else // our gun lvl got messed up and is undefined
				{
					gi.cprintf (ent, PRINT_CHAT, "That's weird. Your Machinegun upgrade level is undefined.\n");
				}


		}
		else // we are Heavy but dont have a machingun
		{	
			if(ent->client->pers.cash >= 100)
			{
				ent->client->pers.cash -= 100;
				Cmd_BuyMachinegun(ent);
			}
			else
				gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
		}
	}
	else // if we are a demo
	if(ent->client->pers.playerClass == CLASS_DEMO)
	{
		// if we have a grenade launcher
		if(ent->client->pers.inventory[grenadeLauncherIndex] > 0 )
		{
			// if we have a lvl 0 gun
			if(ent->client->pers.tierOneUpgradeLevel == 0)
			{
				// upgrade to 1
				if(ent->client->pers.cash >= 200)
				{
					ent->client->pers.cash -= 200;
					ent->client->pers.tierOneUpgradeLevel = 1;
					gi.cprintf (ent, PRINT_CHAT, "You upgraded your Grenade Launcher to LVL 1.\n");
				}
				else
					gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
			}
			else // if we have a lvl 1 gun
			if(ent->client->pers.tierOneUpgradeLevel == 1)
			{
				//upgrade to 2
				if(ent->client->pers.cash >= 300)
				{
					ent->client->pers.cash -= 300;
					ent->client->pers.tierOneUpgradeLevel = 2;
					gi.cprintf (ent, PRINT_CHAT, "You upgraded your Grenade Launcher to LVL 2.\n");
				}
				else
					gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
			}
			else // if we have a lvl 2 gun
			if(ent->client->pers.tierOneUpgradeLevel == 2)
			{
				//upgrade to 3
				if(ent->client->pers.cash >= 400)
				{
					ent->client->pers.cash -= 400;
					ent->client->pers.tierOneUpgradeLevel = 3;
					gi.cprintf (ent, PRINT_CHAT, "You upgraded your Grenade Launcher to LVL 3.\n");
				}
				else
					gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
			}
			else // if our gun is lvl 3 or higher
			if(ent->client->pers.tierOneUpgradeLevel >= 3)
			{
				gi.cprintf (ent, PRINT_CHAT, "Your Grenade Launcher upgrades are maxed out!\n");
			}
			else // our gun lvl got messed up and is undefined
			{
				gi.cprintf (ent, PRINT_CHAT, "That's weird. Your Grenade Launcher upgrade level is undefined.\n");
			}

		}
		else // we are demo but dont have a grenade launcher
		{
			if(ent->client->pers.cash >= 100)
			{
				ent->client->pers.cash -= 100;
				Cmd_BuyGrenadeLauncher(ent);
			}
			else
				gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
		}
			
	}
	else // we are not Support, Heavy, or Demo
	{
		gi.cprintf (ent, PRINT_CHAT, "You need to set your class first dummy!\n");
	}
	
}

void Cmd_BuyTierTwo(edict_t *ent)
{
	gitem_t	*weapon;
	int superShotgunIndex;
	int chaingunIndex;
	int rocketLauncherIndex;

	weapon = FindItem("Super Shotgun");
	superShotgunIndex = ITEM_INDEX(weapon);
	weapon = FindItem("Chaingun");
	chaingunIndex = ITEM_INDEX(weapon);
	weapon = FindItem("Rocket Launcher"); 
	rocketLauncherIndex = ITEM_INDEX(weapon);

	// if our class is support
	if(ent->client->pers.playerClass == CLASS_SUPPORT)
	{
		// if we have a Super Shotgun
		if( ent->client->pers.inventory[superShotgunIndex] > 0 )
		{
			// if our gun is lvl 0
			if(ent->client->pers.tierTwoUpgradeLevel == 0)
			{
				// upgrade to 1
				if(ent->client->pers.cash >= 300)
				{
					ent->client->pers.cash -= 300;
					ent->client->pers.tierTwoUpgradeLevel = 1;
					gi.cprintf (ent, PRINT_CHAT, "You upgraded your Super Shotgun to LVL 1.\n");
				}
				else
					gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
			}
			else // if our gun is lvl 1
			if(ent->client->pers.tierTwoUpgradeLevel == 1)
			{
				// upgrade to 2
				if(ent->client->pers.cash >= 400)
				{
					ent->client->pers.cash -= 400;
					ent->client->pers.tierTwoUpgradeLevel = 2;
					gi.cprintf (ent, PRINT_CHAT, "You upgraded your Super Shotgun to LVL 2.\n");
				}
				else
					gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
			}
			else // if our gun is lvl 2
			if(ent->client->pers.tierTwoUpgradeLevel == 2)
			{
				// upgrade to 3
				if(ent->client->pers.cash >= 500)
				{
					ent->client->pers.cash -= 500;
					ent->client->pers.tierTwoUpgradeLevel = 3;
					gi.cprintf (ent, PRINT_CHAT, "You upgraded your Super Shotgun to LVL 3.\n");
				}
				else
					gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
			}
			else // if your gun is lvl 3 or higher
			if(ent->client->pers.tierTwoUpgradeLevel >= 3)
			{
				gi.cprintf (ent, PRINT_CHAT, "Your Super Shotgun upgrades are maxed out!\n");
			}
			else  // our gun lvl is undefined
			{
				gi.cprintf (ent, PRINT_CHAT, "That's weird. Your Super Shotgun upgrade level is undefined.\n");
			}

		}
		else // if we dont have a Super Shotgun
		{
			if(ent->client->pers.cash >= 200)
			{
				ent->client->pers.cash -= 200;
				Cmd_BuySuperShotgun(ent);
			}
			else
				gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
		}

	}
	else // if we are a Heavy
	if( ent->client->pers.playerClass == CLASS_HEAVY)
	{
		// if we have a Chaingun
		if( ent->client->pers.inventory[chaingunIndex] > 0 )
		{
			// if our gun is lvl 0
			if(ent->client->pers.tierTwoUpgradeLevel == 0)
			{
				// upgrade to 1
				if(ent->client->pers.cash >= 300)
				{
					ent->client->pers.cash -= 300;
					ent->client->pers.tierTwoUpgradeLevel = 1;
					gi.cprintf (ent, PRINT_CHAT, "You upgraded your Chaingun to LVL 1.\n");
				}
				else
					gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
			}
			else // if our gun is lvl 1
			if(ent->client->pers.tierTwoUpgradeLevel == 1)
			{
				// upgrade to 2
				if(ent->client->pers.cash >= 400)
				{
					ent->client->pers.cash -= 400;
					ent->client->pers.tierTwoUpgradeLevel = 2;
					gi.cprintf (ent, PRINT_CHAT, "You upgraded your Chaingun to LVL 2.\n");
				}
				else
					gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
			}
			else // if our gun is lvl 2
			if(ent->client->pers.tierTwoUpgradeLevel == 2)
			{
				// upgrade to 3
				if(ent->client->pers.cash >= 500)
				{
					ent->client->pers.cash -= 500;
					ent->client->pers.tierTwoUpgradeLevel = 3;
					gi.cprintf (ent, PRINT_CHAT, "You upgraded your Chaingun to LVL 3.\n");
				}
				else
					gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
			}
			else // if your gun is lvl 3 or higher
			if(ent->client->pers.tierTwoUpgradeLevel >= 3)
			{
				gi.cprintf (ent, PRINT_CHAT, "Your Chaingun upgrades are maxed out!\n");
			}
			else  // our gun lvl is undefined
			{
				gi.cprintf (ent, PRINT_CHAT, "That's weird. Your Chaingun upgrade level is undefined.\n");
			}

		}
		else // we are Heavy but dont have a Chaingun
		{
			if(ent->client->pers.cash >= 200)
			{
				ent->client->pers.cash -= 200;
				Cmd_BuyChaingun(ent);
			}
			else
				gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
		}

	}
	else // if we are Demo
	if(ent->client->pers.playerClass == CLASS_DEMO)
	{
		// if we have a Rocket Launcher
		if( ent->client->pers.inventory[rocketLauncherIndex] > 0 )
		{
			// if our gun is lvl 0
			if(ent->client->pers.tierTwoUpgradeLevel == 0)
			{
				// upgrade to 1
				if(ent->client->pers.cash >= 300)
				{
					ent->client->pers.cash -= 300;
					ent->client->pers.tierTwoUpgradeLevel = 1;
					gi.cprintf (ent, PRINT_CHAT, "You upgraded your Rocket Launcher to LVL 1.\n");
				}
				else
					gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
			}
			else // if our gun is lvl 1
			if(ent->client->pers.tierTwoUpgradeLevel == 1)
			{
				// upgrade to 2
				if(ent->client->pers.cash >= 400)
				{
					ent->client->pers.cash -= 400;
					ent->client->pers.tierTwoUpgradeLevel = 2;
					gi.cprintf (ent, PRINT_CHAT, "You upgraded your Rocket Launcher to LVL 2.\n");
				}
				else
				gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
			}
			else // if our gun is lvl 2
			if(ent->client->pers.tierTwoUpgradeLevel == 2)
			{
				// upgrade to 3
				if(ent->client->pers.cash >= 500)
				{
					ent->client->pers.cash -= 500;
					ent->client->pers.tierTwoUpgradeLevel = 3;
					gi.cprintf (ent, PRINT_CHAT, "You upgraded your Rocket Launcher to LVL 3.\n");
				}
				else
					gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
			}
			else // if your gun is lvl 3 or higher
			if(ent->client->pers.tierTwoUpgradeLevel >= 3)
			{
				gi.cprintf (ent, PRINT_CHAT, "Your Rocket Launcher upgrades are maxed out!\n");
			}
			else  // our gun lvl is undefined
			{
				gi.cprintf (ent, PRINT_CHAT, "That's weird. Your Rocket Launcher upgrade level is undefined.\n");
			}
		}
		else // we are Demo but dont have a rocket launcher
		{
			if(ent->client->pers.cash >= 200)
			{
				ent->client->pers.cash -= 200;
				Cmd_BuyRocketLauncher(ent);
			}
			else
				gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
		}
	}
	else // we are not Support, Heavy, or Demo
	{
		gi.cprintf (ent, PRINT_CHAT, "You need to set your class first dummy!\n");
	}
	
}

void Cmd_BuyShells(edict_t *ent)
{
	gitem_t		*ammo;
	int			ammoIndex;

	ammo = FindItem("Shells");
	ammoIndex = ITEM_INDEX(ammo);
	ent->client->pers.inventory[ammoIndex] = 999; 

	gi.cprintf (ent, PRINT_CHAT, "You just bought Shells.\n");
}

void Cmd_BuyBullets(edict_t *ent)
{
	gitem_t		*ammo;
	int			ammoIndex;

	ammo = FindItem("Bullets");
	ammoIndex = ITEM_INDEX(ammo);
	ent->client->pers.inventory[ammoIndex] = 999;

	gi.cprintf (ent, PRINT_CHAT, "You just bought Bullets.\n");
}

void Cmd_BuyGrenades(edict_t *ent)
{
	gitem_t		*ammo;
	int			ammoIndex;

	ammo = FindItem("Grenades");
	ammoIndex = ITEM_INDEX(ammo);
	ent->client->pers.inventory[ammoIndex] = 999;

	gi.cprintf (ent, PRINT_CHAT, "You just bought Grenades.\n");
}

void Cmd_BuyRockets(edict_t *ent)
{
	gitem_t		*ammo;
	int			ammoIndex;

	ammo = FindItem("Rockets");
	ammoIndex = ITEM_INDEX(ammo);
	ent->client->pers.inventory[ammoIndex] = 999;

	gi.cprintf (ent, PRINT_CHAT, "You just bought Rockets.\n");
}

void Cmd_BuyClassAmmo(edict_t *ent)
{
	if(ent->client->pers.playerClass == CLASS_SUPPORT)
	{
		if(ent->client->pers.cash >= 50)
		{
			ent->client->pers.cash -= 50;
			Cmd_BuyShells(ent);
		}
		else
			gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
	}
	else
	if( ent->client->pers.playerClass == CLASS_HEAVY)
	{
		if(ent->client->pers.cash >= 25)
		{
			ent->client->pers.cash -= 25;
			Cmd_BuyBullets(ent);
		}
		else
			gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
	}
	else
	if(ent->client->pers.playerClass == CLASS_DEMO)
	{
		if(ent->client->pers.cash >= 50)
		{
			ent->client->pers.cash -= 50;
			Cmd_BuyGrenades(ent);
			Cmd_BuyRockets(ent);
		}
		else
			gi.cprintf (ent, PRINT_CHAT, "Not enough dosh.\n");
	}
	else
	{
		gi.cprintf (ent, PRINT_CHAT, "You need to set your class first dummy!\n");
	}
}

void Cmd_LoadsOfMoney(edict_t *ent)
{
	ent->client->pers.cash = 99999;
}

/*
=================
ClientCommand
=================
*/
void ClientCommand (edict_t *ent)
{
	char	*cmd;

	if (!ent->client)
		return;		// not fully in game yet

	cmd = gi.argv(0);

	if (Q_stricmp (cmd, "players") == 0)
	{
		Cmd_Players_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "say") == 0)
	{
		Cmd_Say_f (ent, false, false);
		return;
	}
	if (Q_stricmp (cmd, "say_team") == 0)
	{
		Cmd_Say_f (ent, true, false);
		return;
	}
	if (Q_stricmp (cmd, "score") == 0)
	{
		Cmd_Score_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "help") == 0)
	{
		Cmd_Help_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "trader") == 0)
	{
		Cmd_Trader_f (ent);
		return;
	}

	if (level.intermissiontime)
		return;

	if (Q_stricmp (cmd, "use") == 0)
		Cmd_Use_f (ent);
	else if (Q_stricmp (cmd, "drop") == 0)
		Cmd_Drop_f (ent);
	else if (Q_stricmp (cmd, "give") == 0)
		Cmd_Give_f (ent);
	else if (Q_stricmp (cmd, "god") == 0)
		Cmd_God_f (ent);
	else if (Q_stricmp (cmd, "notarget") == 0)
		Cmd_Notarget_f (ent);
	else if (Q_stricmp (cmd, "noclip") == 0)
		Cmd_Noclip_f (ent);
	else if (Q_stricmp (cmd, "inven") == 0)
		Cmd_Inven_f (ent);
	else if (Q_stricmp (cmd, "invnext") == 0)
		SelectNextItem (ent, -1);
	else if (Q_stricmp (cmd, "invprev") == 0)
		SelectPrevItem (ent, -1);
	else if (Q_stricmp (cmd, "invnextw") == 0)
		SelectNextItem (ent, IT_WEAPON);
	else if (Q_stricmp (cmd, "invprevw") == 0)
		SelectPrevItem (ent, IT_WEAPON);
	else if (Q_stricmp (cmd, "invnextp") == 0)
		SelectNextItem (ent, IT_POWERUP);
	else if (Q_stricmp (cmd, "invprevp") == 0)
		SelectPrevItem (ent, IT_POWERUP);
	else if (Q_stricmp (cmd, "invuse") == 0)
		Cmd_InvUse_f (ent);
	else if (Q_stricmp (cmd, "invdrop") == 0)
		Cmd_InvDrop_f (ent);
	else if (Q_stricmp (cmd, "weapprev") == 0)
		Cmd_WeapPrev_f (ent);
	else if (Q_stricmp (cmd, "weapnext") == 0)
		Cmd_WeapNext_f (ent);
	else if (Q_stricmp (cmd, "weaplast") == 0)
		Cmd_WeapLast_f (ent);
	else if (Q_stricmp (cmd, "kill") == 0)
		Cmd_Kill_f (ent);
	else if (Q_stricmp (cmd, "putaway") == 0)
		Cmd_PutAway_f (ent);
	else if (Q_stricmp (cmd, "wave") == 0)
		Cmd_Wave_f (ent);
	else if (Q_stricmp(cmd, "playerlist") == 0)
		Cmd_PlayerList_f(ent);
	else if (Q_stricmp(cmd, "setClass") == 0)
		Cmd_SetClass(ent);
	else if (Q_stricmp(cmd, "printClass") == 0)
		Cmd_PrintClass(ent);
	else if (Q_stricmp(cmd, "buyShotgun") == 0)
		Cmd_BuyShotgun(ent);
	else if (Q_stricmp(cmd, "buySuperShotgun") == 0)
		Cmd_BuySuperShotgun(ent);
	else if (Q_stricmp(cmd, "buyMachinegun") == 0)
		Cmd_BuyMachinegun(ent);
	else if (Q_stricmp(cmd, "buyChaingun") == 0)
		Cmd_BuyChaingun(ent);
	else if (Q_stricmp(cmd, "buyGrenadeLauncher") == 0)
		Cmd_BuyGrenadeLauncher(ent);
	else if (Q_stricmp(cmd, "buyRocketLauncher") == 0)
		Cmd_BuyRocketLauncher(ent);
	else if (Q_stricmp(cmd, "buyTierOne") == 0)
		Cmd_BuyTierOne(ent);
	else if (Q_stricmp(cmd, "buyTierTwo") == 0)
		Cmd_BuyTierTwo(ent);
	else if (Q_stricmp(cmd, "buyShells") == 0)
		Cmd_BuyShells(ent);
	else if (Q_stricmp(cmd, "buyBullets") == 0)
		Cmd_BuyBullets(ent);
	else if (Q_stricmp(cmd, "buyGrenades") == 0)
		Cmd_BuyGrenades(ent);
	else if (Q_stricmp(cmd, "buyRockets") == 0)
		Cmd_BuyRockets(ent);
	else if (Q_stricmp(cmd, "buyClassAmmo") == 0)
		Cmd_BuyClassAmmo(ent);
	else if (Q_stricmp(cmd, "loadsOfMoney") == 0)
		Cmd_LoadsOfMoney(ent);
	else	// anything that doesn't match a command will be a chat
		Cmd_Say_f (ent, false, true);
}
