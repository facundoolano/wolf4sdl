// WL_MAIN.C

#include <unistd.h>
#include "wl_def.h"
#pragma hdrstop
#include <SDL_syswm.h>


/*
=============================================================================

                             WOLFENSTEIN 3-D

                        An Id Software production

                             by John Carmack

=============================================================================
*/

extern byte signon[];

/*
=============================================================================

                             LOCAL CONSTANTS

=============================================================================
*/


#define FOCALLENGTH     (0x5700l)               // in global coordinates
#define VIEWGLOBAL      0x10000                 // globals visable flush to wall

#define VIEWWIDTH       256                     // size of view window
#define VIEWHEIGHT      144

/*
=============================================================================

                            GLOBAL VARIABLES

=============================================================================
*/

char    str[80];
int     dirangle[9] = {0,ANGLES/8,2*ANGLES/8,3*ANGLES/8,4*ANGLES/8,
                       5*ANGLES/8,6*ANGLES/8,7*ANGLES/8,ANGLES};

//
// proejection variables
//
fixed    focallength;
unsigned screenofs;
int      viewscreenx, viewscreeny;
int      viewwidth;
int      viewheight;
short    centerx;
int      shootdelta;           // pixels away from centerx a target can be
fixed    scale;
int32_t  heightnumerator;


void    Quit (const char *error,...);

boolean startgame;
boolean loadedgame;
int     mouseadjustment;

char    configdir[256] = "";
char    configname[13] = "config.";

//
// Command line parameter variables
//
boolean param_debugmode = false;
boolean param_nowait = false;
int     param_difficulty = 1;           // default is "normal"
int     param_tedlevel = -1;            // default is not to start a level
int     param_joystickindex = 0;

int     param_joystickhat = -1;
int     param_samplerate = 44100;
int     param_audiobuffer = 2048 / (44100 / param_samplerate);

int     param_mission = 0;
boolean param_goodtimes = false;
boolean param_ignorenumchunks = false;

/*
=============================================================================

                            LOCAL VARIABLES

=============================================================================
*/


/*
====================
=
= ReadConfig
=
====================
*/

void ReadConfig(void)
{
    SDMode  sd;
    SMMode  sm;
    SDSMode sds;

    char configpath[300];


    if(configdir[0])
        snprintf(configpath, sizeof(configpath), "%s/%s", configdir, configname);
    else
        strcpy(configpath, configname);

    const int file = open(configpath, O_RDONLY | O_BINARY);
    if (file != -1)
    {
        //
        // valid config file
        //
        word tmp;
        read(file,&tmp,sizeof(tmp));
        if(tmp!=0xfefa)
        {
            close(file);
            goto noconfig;
        }
        read(file,Scores,sizeof(HighScore) * MaxScores);

        read(file,&sd,sizeof(sd));
        read(file,&sm,sizeof(sm));
        read(file,&sds,sizeof(sds));

        read(file,&mouseenabled,sizeof(mouseenabled));
        read(file,&joystickenabled,sizeof(joystickenabled));
        boolean dummyJoypadEnabled;
        read(file,&dummyJoypadEnabled,sizeof(dummyJoypadEnabled));
        boolean dummyJoystickProgressive;
        read(file,&dummyJoystickProgressive,sizeof(dummyJoystickProgressive));
        int dummyJoystickPort = 0;
        read(file,&dummyJoystickPort,sizeof(dummyJoystickPort));

        read(file,dirscan,sizeof(dirscan));
        read(file,buttonscan,sizeof(buttonscan));
        read(file,buttonmouse,sizeof(buttonmouse));
        read(file,buttonjoy,sizeof(buttonjoy));

        read(file,&viewsize,sizeof(viewsize));
        read(file,&mouseadjustment,sizeof(mouseadjustment));

        close(file);

        if ((sd == sdm_AdLib || sm == smm_AdLib) && !AdLibPresent
                && !SoundBlasterPresent)
        {
            sd = sdm_PC;
            sm = smm_Off;
        }

        if ((sds == sds_SoundBlaster && !SoundBlasterPresent))
            sds = sds_Off;

        // make sure values are correct

        if(mouseenabled) mouseenabled=true;
        if(joystickenabled) joystickenabled=true;

        if (!MousePresent)
            mouseenabled = false;
        if (!IN_JoyPresent())
            joystickenabled = false;

        if(mouseadjustment<0) mouseadjustment=0;
        else if(mouseadjustment>9) mouseadjustment=9;

        if(viewsize<4) viewsize=4;
        else if(viewsize>21) viewsize=21;

        MainMenu[6].active=1;
        MainItems.curpos=0;
    }
    else
    {
        //
        // no config file, so select by hardware
        //
noconfig:
        if (SoundBlasterPresent || AdLibPresent)
        {
            sd = sdm_AdLib;
            sm = smm_AdLib;
        }
        else
        {
            sd = sdm_PC;
            sm = smm_Off;
        }

        if (SoundBlasterPresent)
            sds = sds_SoundBlaster;
        else
            sds = sds_Off;

        if (MousePresent)
            mouseenabled = true;

        if (IN_JoyPresent())
            joystickenabled = true;

        viewsize = 19;                          // start with a good size
        mouseadjustment=5;
    }

    SD_SetMusicMode (sm);
    SD_SetSoundMode (sd);
    SD_SetDigiDevice (sds);
}

/*
====================
=
= WriteConfig
=
====================
*/

void WriteConfig(void)
{
    char configpath[300];


    if(configdir[0])
        snprintf(configpath, sizeof(configpath), "%s/%s", configdir, configname);
    else
        strcpy(configpath, configname);

    const int file = open(configpath, O_CREAT | O_WRONLY | O_BINARY, 0644);
    if (file != -1)
    {
        word tmp=0xfefa;
        write(file,&tmp,sizeof(tmp));
        write(file,Scores,sizeof(HighScore) * MaxScores);

        write(file,&SoundMode,sizeof(SoundMode));
        write(file,&MusicMode,sizeof(MusicMode));
        write(file,&DigiMode,sizeof(DigiMode));

        write(file,&mouseenabled,sizeof(mouseenabled));
        write(file,&joystickenabled,sizeof(joystickenabled));
        boolean dummyJoypadEnabled = false;
        write(file,&dummyJoypadEnabled,sizeof(dummyJoypadEnabled));
        boolean dummyJoystickProgressive = false;
        write(file,&dummyJoystickProgressive,sizeof(dummyJoystickProgressive));
        int dummyJoystickPort = 0;
        write(file,&dummyJoystickPort,sizeof(dummyJoystickPort));

        write(file,dirscan,sizeof(dirscan));
        write(file,buttonscan,sizeof(buttonscan));
        write(file,buttonmouse,sizeof(buttonmouse));
        write(file,buttonjoy,sizeof(buttonjoy));

        write(file,&viewsize,sizeof(viewsize));
        write(file,&mouseadjustment,sizeof(mouseadjustment));

        close(file);
    }
}


//===========================================================================

/*
=====================
=
= NewGame
=
= Set up new game to start from the beginning
=
=====================
*/

void NewGame (int difficulty,int episode)
{
    memset (&gamestate,0,sizeof(gamestate));
    gamestate.difficulty = difficulty;
    gamestate.weapon = gamestate.bestweapon
            = gamestate.chosenweapon = wp_pistol;
    gamestate.health = 100;
    gamestate.ammo = STARTAMMO;
    gamestate.lives = 3;
    gamestate.nextextra = EXTRAPOINTS;
    gamestate.episode=episode;

    startgame = true;
}

//===========================================================================

void DiskFlopAnim(int x,int y)
{
    static int8_t which=0;
    if (!x && !y)
        return;
    VWB_DrawPic(x,y,C_DISKLOADING1PIC+which);
    VW_UpdateScreen();
    which^=1;
}


int32_t DoChecksum(byte *source,unsigned size,int32_t checksum)
{
    unsigned i;

    for (i=0;i<size-1;i++)
    checksum += source[i]^source[i+1];

    return checksum;
}


/*
==================
=
= SaveTheGame
=
==================
*/

extern statetype s_grdstand;
extern statetype s_player;

boolean SaveTheGame(FILE *file,int x,int y)
{
//    struct diskfree_t dfree;
//    int32_t avail,size,checksum;
    int checksum;
    objtype *ob;
    objtype nullobj;
    statobj_t nullstat;

/*    if (_dos_getdiskfree(0,&dfree))
        Quit("Error in _dos_getdiskfree call");

    avail = (int32_t)dfree.avail_clusters *
                  dfree.bytes_per_sector *
                  dfree.sectors_per_cluster;

    size = 0;
    for (ob = player; ob ; ob=ob->next)
        size += sizeof(*ob);
    size += sizeof(nullobj);

    size += sizeof(gamestate) +
            sizeof(LRstruct)*LRpack +
            sizeof(tilemap) +
            sizeof(actorat) +
            sizeof(laststatobj) +
            sizeof(statobjlist) +
            sizeof(doorposition) +
            sizeof(pwallstate) +
            sizeof(pwalltile) +
            sizeof(pwallx) +
            sizeof(pwally) +
            sizeof(pwalldir) +
            sizeof(pwallpos);

    if (avail < size)
    {
        Message(STR_NOSPACE1"\n"STR_NOSPACE2);
        return false;
    }*/

    checksum = 0;

    DiskFlopAnim(x,y);
    fwrite(&gamestate,sizeof(gamestate),1,file);
    checksum = DoChecksum((byte *)&gamestate,sizeof(gamestate),checksum);

    DiskFlopAnim(x,y);
    fwrite(&LevelRatios[0],sizeof(LRstruct)*LRpack,1,file);
    checksum = DoChecksum((byte *)&LevelRatios[0],sizeof(LRstruct)*LRpack,checksum);

    DiskFlopAnim(x,y);
    fwrite(tilemap,sizeof(tilemap),1,file);
    checksum = DoChecksum((byte *)tilemap,sizeof(tilemap),checksum);
    DiskFlopAnim(x,y);

    int i;
    for(i=0;i<MAPSIZE;i++)
    {
        for(int j=0;j<MAPSIZE;j++)
        {
            word actnum;
            objtype *objptr=actorat[i][j];
            if(ISPOINTER(objptr))
                actnum=0x8000 | (word)(objptr-objlist);
            else
                actnum=(word)(uintptr_t)objptr;
            fwrite(&actnum,sizeof(actnum),1,file);
            checksum = DoChecksum((byte *)&actnum,sizeof(actnum),checksum);
        }
    }

    fwrite (areaconnect,sizeof(areaconnect),1,file);
    fwrite (areabyplayer,sizeof(areabyplayer),1,file);

    // player object needs special treatment as it's in WL_AGENT.CPP and not in
    // WL_ACT2.CPP which could cause problems for the relative addressing

    ob = player;
    DiskFlopAnim(x,y);
    memcpy(&nullobj,ob,sizeof(nullobj));
    nullobj.state=(statetype *) ((uintptr_t)nullobj.state-(uintptr_t)&s_player);
    fwrite(&nullobj,sizeof(nullobj),1,file);
    ob = ob->next;

    DiskFlopAnim(x,y);
    for (; ob ; ob=ob->next)
    {
        memcpy(&nullobj,ob,sizeof(nullobj));
        nullobj.state=(statetype *) ((uintptr_t)nullobj.state-(uintptr_t)&s_grdstand);
        fwrite(&nullobj,sizeof(nullobj),1,file);
    }
    nullobj.active = ac_badobject;          // end of file marker
    DiskFlopAnim(x,y);
    fwrite(&nullobj,sizeof(nullobj),1,file);

    DiskFlopAnim(x,y);
    word laststatobjnum=(word) (laststatobj-statobjlist);
    fwrite(&laststatobjnum,sizeof(laststatobjnum),1,file);
    checksum = DoChecksum((byte *)&laststatobjnum,sizeof(laststatobjnum),checksum);

    DiskFlopAnim(x,y);
    for(i=0;i<MAXSTATS;i++)
    {
        memcpy(&nullstat,statobjlist+i,sizeof(nullstat));
        nullstat.visspot=(byte *) ((uintptr_t) nullstat.visspot-(uintptr_t)spotvis);
        fwrite(&nullstat,sizeof(nullstat),1,file);
        checksum = DoChecksum((byte *)&nullstat,sizeof(nullstat),checksum);
    }

    DiskFlopAnim(x,y);
    fwrite (doorposition,sizeof(doorposition),1,file);
    checksum = DoChecksum((byte *)doorposition,sizeof(doorposition),checksum);
    DiskFlopAnim(x,y);
    fwrite (doorobjlist,sizeof(doorobjlist),1,file);
    checksum = DoChecksum((byte *)doorobjlist,sizeof(doorobjlist),checksum);

    DiskFlopAnim(x,y);
    fwrite (&pwallstate,sizeof(pwallstate),1,file);
    checksum = DoChecksum((byte *)&pwallstate,sizeof(pwallstate),checksum);
    fwrite (&pwalltile,sizeof(pwalltile),1,file);
    checksum = DoChecksum((byte *)&pwalltile,sizeof(pwalltile),checksum);
    fwrite (&pwallx,sizeof(pwallx),1,file);
    checksum = DoChecksum((byte *)&pwallx,sizeof(pwallx),checksum);
    fwrite (&pwally,sizeof(pwally),1,file);
    checksum = DoChecksum((byte *)&pwally,sizeof(pwally),checksum);
    fwrite (&pwalldir,sizeof(pwalldir),1,file);
    checksum = DoChecksum((byte *)&pwalldir,sizeof(pwalldir),checksum);
    fwrite (&pwallpos,sizeof(pwallpos),1,file);
    checksum = DoChecksum((byte *)&pwallpos,sizeof(pwallpos),checksum);

    //
    // WRITE OUT CHECKSUM
    //
    fwrite (&checksum,sizeof(checksum),1,file);

    fwrite (&lastgamemusicoffset,sizeof(lastgamemusicoffset),1,file);

    return(true);
}

//===========================================================================

/*
==================
=
= LoadTheGame
=
==================
*/

boolean LoadTheGame(FILE *file,int x,int y)
{
    int32_t checksum,oldchecksum;
    objtype nullobj;
    statobj_t nullstat;

    checksum = 0;

    DiskFlopAnim(x,y);
    fread (&gamestate,sizeof(gamestate),1,file);
    checksum = DoChecksum((byte *)&gamestate,sizeof(gamestate),checksum);

    DiskFlopAnim(x,y);
    fread (&LevelRatios[0],sizeof(LRstruct)*LRpack,1,file);
    checksum = DoChecksum((byte *)&LevelRatios[0],sizeof(LRstruct)*LRpack,checksum);

    DiskFlopAnim(x,y);
    SetupGameLevel ();

    DiskFlopAnim(x,y);
    fread (tilemap,sizeof(tilemap),1,file);
    checksum = DoChecksum((byte *)tilemap,sizeof(tilemap),checksum);

    DiskFlopAnim(x,y);

    int actnum=0, i;
    for(i=0;i<MAPSIZE;i++)
    {
        for(int j=0;j<MAPSIZE;j++)
        {
            fread (&actnum,sizeof(word),1,file);
            checksum = DoChecksum((byte *) &actnum,sizeof(word),checksum);
            if(actnum&0x8000)
                actorat[i][j]=objlist+(actnum&0x7fff);
            else
                actorat[i][j]=(objtype *)(uintptr_t) actnum;
        }
    }

    fread (areaconnect,sizeof(areaconnect),1,file);
    fread (areabyplayer,sizeof(areabyplayer),1,file);

    InitActorList ();
    DiskFlopAnim(x,y);
    fread (player,sizeof(*player),1,file);
    player->state=(statetype *) ((uintptr_t)player->state+(uintptr_t)&s_player);

    while (1)
    {
        DiskFlopAnim(x,y);
        fread (&nullobj,sizeof(nullobj),1,file);
        if (nullobj.active == ac_badobject)
            break;
        GetNewActor ();
        nullobj.state=(statetype *) ((uintptr_t)nullobj.state+(uintptr_t)&s_grdstand);
        // don't copy over the links
        memcpy (newobj,&nullobj,sizeof(nullobj)-8);
    }

    DiskFlopAnim(x,y);
    word laststatobjnum;
    fread (&laststatobjnum,sizeof(laststatobjnum),1,file);
    laststatobj=statobjlist+laststatobjnum;
    checksum = DoChecksum((byte *)&laststatobjnum,sizeof(laststatobjnum),checksum);

    DiskFlopAnim(x,y);
    for(i=0;i<MAXSTATS;i++)
    {
        fread(&nullstat,sizeof(nullstat),1,file);
        checksum = DoChecksum((byte *)&nullstat,sizeof(nullstat),checksum);
        nullstat.visspot=(byte *) ((uintptr_t)nullstat.visspot+(uintptr_t)spotvis);
        memcpy(statobjlist+i,&nullstat,sizeof(nullstat));
    }

    DiskFlopAnim(x,y);
    fread (doorposition,sizeof(doorposition),1,file);
    checksum = DoChecksum((byte *)doorposition,sizeof(doorposition),checksum);
    DiskFlopAnim(x,y);
    fread (doorobjlist,sizeof(doorobjlist),1,file);
    checksum = DoChecksum((byte *)doorobjlist,sizeof(doorobjlist),checksum);

    DiskFlopAnim(x,y);
    fread (&pwallstate,sizeof(pwallstate),1,file);
    checksum = DoChecksum((byte *)&pwallstate,sizeof(pwallstate),checksum);
    fread (&pwalltile,sizeof(pwalltile),1,file);
    checksum = DoChecksum((byte *)&pwalltile,sizeof(pwalltile),checksum);
    fread (&pwallx,sizeof(pwallx),1,file);
    checksum = DoChecksum((byte *)&pwallx,sizeof(pwallx),checksum);
    fread (&pwally,sizeof(pwally),1,file);
    checksum = DoChecksum((byte *)&pwally,sizeof(pwally),checksum);
    fread (&pwalldir,sizeof(pwalldir),1,file);
    checksum = DoChecksum((byte *)&pwalldir,sizeof(pwalldir),checksum);
    fread (&pwallpos,sizeof(pwallpos),1,file);
    checksum = DoChecksum((byte *)&pwallpos,sizeof(pwallpos),checksum);

    if (gamestate.secretcount)      // assign valid floorcodes under moved pushwalls
    {
        word *map, *obj; word tile, sprite;
        map = mapsegs[0]; obj = mapsegs[1];
        for (y=0;y<mapheight;y++)
            for (x=0;x<mapwidth;x++)
            {
                tile = *map++; sprite = *obj++;
                if (sprite == PUSHABLETILE && !tilemap[x][y]
                    && (tile < AREATILE || tile >= (AREATILE+NUMMAPS)))
                {
                    if (*map >= AREATILE)
                        tile = *map;
                    if (*(map-1-mapwidth) >= AREATILE)
                        tile = *(map-1-mapwidth);
                    if (*(map-1+mapwidth) >= AREATILE)
                        tile = *(map-1+mapwidth);
                    if ( *(map-2) >= AREATILE)
                        tile = *(map-2);

                    *(map-1) = tile; *(obj-1) = 0;
                }
            }
    }

    Thrust(0,0);    // set player->areanumber to the floortile you're standing on

    fread (&oldchecksum,sizeof(oldchecksum),1,file);

    fread (&lastgamemusicoffset,sizeof(lastgamemusicoffset),1,file);
    if(lastgamemusicoffset<0) lastgamemusicoffset=0;


    if (oldchecksum != checksum)
    {
        Message(STR_SAVECHT1"\n"
                STR_SAVECHT2"\n"
                STR_SAVECHT3"\n"
                STR_SAVECHT4);

        IN_ClearKeysDown();
        IN_Ack();

        gamestate.oldscore = gamestate.score = 0;
        gamestate.lives = 1;
        gamestate.weapon =
            gamestate.chosenweapon =
            gamestate.bestweapon = wp_pistol;
        gamestate.ammo = 8;
    }

    return true;
}

//===========================================================================

/*
==========================
=
= ShutdownId
=
= Shuts down all ID_?? managers
=
==========================
*/

void ShutdownId (void)
{
    US_Shutdown ();         // This line is completely useless...
    SD_Shutdown ();
    PM_Shutdown ();
    IN_Shutdown ();
    VW_Shutdown ();
    CA_Shutdown ();
}


//===========================================================================

/*
==================
=
= BuildTables
=
= Calculates:
=
= scale                 projection constant
= sintable/costable     overlapping fractional tables
=
==================
*/

const float radtoint = (float)(FINEANGLES/2/PI);

void BuildTables (void)
{
    //
    // calculate fine tangents
    //

    int i;
    for(i=0;i<FINEANGLES/8;i++)
    {
        double tang=tan((i+0.5)/radtoint);
        finetangent[i]=(int32_t)(tang*GLOBAL1);
        finetangent[FINEANGLES/4-1-i]=(int32_t)((1/tang)*GLOBAL1);
    }

    //
    // costable overlays sintable with a quarter phase shift
    // ANGLES is assumed to be divisable by four
    //

    float angle=0;
    float anglestep=(float)(PI/2/ANGLEQUAD);
    for(i=0; i<ANGLEQUAD; i++)
    {
        fixed value=(int32_t)(GLOBAL1*sin(angle));
        sintable[i]=sintable[i+ANGLES]=sintable[ANGLES/2-i]=value;
        sintable[ANGLES-i]=sintable[ANGLES/2+i]=-value;
        angle+=anglestep;
    }
    sintable[ANGLEQUAD] = 65536;
    sintable[3*ANGLEQUAD] = -65536;

}

//===========================================================================


/*
====================
=
= CalcProjection
=
= Uses focallength
=
====================
*/

void CalcProjection (int32_t focal)
{
    int     i;
    int    intang;
    float   angle;
    double  tang;
    int     halfview;
    double  facedist;

    focallength = focal;
    facedist = focal+MINDIST;
    halfview = viewwidth/2;                                 // half view in pixels

    //
    // calculate scale value for vertical height calculations
    // and sprite x calculations
    //
    scale = (fixed) (halfview*facedist/(VIEWGLOBAL/2));

    //
    // divide heightnumerator by a posts distance to get the posts height for
    // the heightbuffer.  The pixel height is height>>2
    //
    heightnumerator = (TILEGLOBAL*scale)>>6;

    //
    // calculate the angle offset from view angle of each pixel's ray
    //

    for (i=0;i<halfview;i++)
    {
        // start 1/2 pixel over, so viewangle bisects two middle pixels
        tang = (int32_t)i*VIEWGLOBAL/viewwidth/facedist;
        angle = (float) atan(tang);
        intang = (int) (angle*radtoint);
        pixelangle[halfview-1-i] = intang;
        pixelangle[halfview+i] = -intang;
    }
}



//===========================================================================

/*
===================
=
= SetupWalls
=
= Map tile values to scaled pics
=
===================
*/

void SetupWalls (void)
{
    int     i;

    horizwall[0]=0;
    vertwall[0]=0;

    for (i=1;i<MAXWALLTILES;i++)
    {
        horizwall[i]=(i-1)*2;
        vertwall[i]=(i-1)*2+1;
    }
}

//===========================================================================

/*
==========================
=
= SignonScreen
=
==========================
*/

void SignonScreen (void)                        // VGA version
{
    VL_SetVGAPlaneMode ();

    VL_MungePic (signon,320,200);
    VL_MemToScreen (signon,320,200,0,0);
}


/*
==========================
=
= FinishSignon
=
==========================
*/

void FinishSignon (void)
{
    VW_Bar (0,189,300,11,VL_GetPixel(0,0));
    WindowX = 0;
    WindowW = 320;
    PrintY = 190;

    SETFONTCOLOR(14,4);

    US_CPrint ("Press a key");


    VH_UpdateScreen();

    if (!param_nowait)
        IN_Ack ();

    VW_Bar (0,189,300,11,VL_GetPixel(0,0));

    PrintY = 190;
    SETFONTCOLOR(10,4);

    US_CPrint ("Working...");

    VH_UpdateScreen();

    SETFONTCOLOR(0,15);
}

//===========================================================================

/*
=====================
=
= InitDigiMap
=
=====================
*/

// channel mapping:
//  -1: any non reserved channel
//   0: player weapons
//   1: boss weapons

static int wolfdigimap[] =
    {
        // These first sounds are in the upload version
        HALTSND,                0,  -1,
        DOGBARKSND,             1,  -1,
        CLOSEDOORSND,           2,  -1,
        OPENDOORSND,            3,  -1,
        ATKMACHINEGUNSND,       4,   0,
        ATKPISTOLSND,           5,   0,
        ATKGATLINGSND,          6,   0,
        SCHUTZADSND,            7,  -1,
        GUTENTAGSND,            8,  -1,
        MUTTISND,               9,  -1,
        BOSSFIRESND,            10,  1,
        SSFIRESND,              11, -1,
        DEATHSCREAM1SND,        12, -1,
        DEATHSCREAM2SND,        13, -1,
        DEATHSCREAM3SND,        13, -1,
        TAKEDAMAGESND,          14, -1,
        PUSHWALLSND,            15, -1,

        LEBENSND,               20, -1,
        NAZIFIRESND,            21, -1,
        SLURPIESND,             22, -1,

        YEAHSND,                32, -1,

#ifndef UPLOAD
        // These are in all other episodes
        DOGDEATHSND,            16, -1,
        AHHHGSND,               17, -1,
        DIESND,                 18, -1,
        EVASND,                 19, -1,

        TOT_HUNDSND,            23, -1,
        MEINGOTTSND,            24, -1,
        SCHABBSHASND,           25, -1,
        HITLERHASND,            26, -1,
        SPIONSND,               27, -1,
        NEINSOVASSND,           28, -1,
        DOGATTACKSND,           29, -1,
        LEVELDONESND,           30, -1,
        MECHSTEPSND,            31, -1,

        SCHEISTSND,             33, -1,
        DEATHSCREAM4SND,        34, -1,         // AIIEEE
        DEATHSCREAM5SND,        35, -1,         // DEE-DEE
        DONNERSND,              36, -1,         // EPISODE 4 BOSS DIE
        EINESND,                37, -1,         // EPISODE 4 BOSS SIGHTING
        ERLAUBENSND,            38, -1,         // EPISODE 6 BOSS SIGHTING
        DEATHSCREAM6SND,        39, -1,         // FART
        DEATHSCREAM7SND,        40, -1,         // GASP
        DEATHSCREAM8SND,        41, -1,         // GUH-BOY!
        DEATHSCREAM9SND,        42, -1,         // AH GEEZ!
        KEINSND,                43, -1,         // EPISODE 5 BOSS SIGHTING
        MEINSND,                44, -1,         // EPISODE 6 BOSS DIE
        ROSESND,                45, -1,         // EPISODE 5 BOSS DIE

#endif
        LASTSOUND
    };


void InitDigiMap (void)
{
    int *map;

    for (map = wolfdigimap; *map != LASTSOUND; map += 3)
    {
        DigiMap[map[0]] = map[1];
        DigiChannel[map[1]] = map[2];
        SD_PrepareSound(map[1]);
    }
}

CP_iteminfo MusicItems={CTL_X,CTL_Y,6,0,32};
CP_itemtype MusicMenu[]=
    {
        {1,"Get Them!",0},
        {1,"Searching",0},
        {1,"P.O.W.",0},
        {1,"Suspense",0},
        {1,"War March",0},
        {1,"Around The Corner!",0},

        {1,"Nazi Anthem",0},
        {1,"Lurking...",0},
        {1,"Going After Hitler",0},
        {1,"Pounding Headache",0},
        {1,"Into the Dungeons",0},
        {1,"Ultimate Conquest",0},

        {1,"Kill the S.O.B.",0},
        {1,"The Nazi Rap",0},
        {1,"Twelfth Hour",0},
        {1,"Zero Hour",0},
        {1,"Ultimate Conquest",0},
        {1,"Wolfpack",0}
    };

void DoJukebox(void)
{
    int which,lastsong=-1;
    unsigned start;
    unsigned songs[]=
        {
            GETTHEM_MUS,
            SEARCHN_MUS,
            POW_MUS,
            SUSPENSE_MUS,
            WARMARCH_MUS,
            CORNER_MUS,

            NAZI_OMI_MUS,
            PREGNANT_MUS,
            GOINGAFT_MUS,
            HEADACHE_MUS,
            DUNGEON_MUS,
            ULTIMATE_MUS,

            INTROCW3_MUS,
            NAZI_RAP_MUS,
            TWELFTH_MUS,
            ZEROHOUR_MUS,
            ULTIMATE_MUS,
            PACMAN_MUS
        };

    IN_ClearKeysDown();
    if (!AdLibPresent && !SoundBlasterPresent)
        return;

    MenuFadeOut();

#ifndef UPLOAD
    start = ((SDL_GetTicks()/10)%3)*6;
#else
    start = 0;
#endif

    CA_CacheGrChunk (STARTFONT+1);
    CacheLump (CONTROLS_LUMP_START,CONTROLS_LUMP_END);
    CA_LoadAllSounds ();

    fontnumber=1;
    ClearMScreen ();
    VWB_DrawPic(112,184,C_MOUSELBACKPIC);
    DrawStripes (10);
    SETFONTCOLOR (TEXTCOLOR,BKGDCOLOR);

    DrawWindow (CTL_X-2,CTL_Y-6,280,13*7,BKGDCOLOR);

    DrawMenu (&MusicItems,&MusicMenu[start]);

    SETFONTCOLOR (READHCOLOR,BKGDCOLOR);
    PrintY=15;
    WindowX = 0;
    WindowY = 320;
    US_CPrint ("Robert's Jukebox");

    SETFONTCOLOR (TEXTCOLOR,BKGDCOLOR);
    VW_UpdateScreen();
    MenuFadeIn();

    do
    {
        which = HandleMenu(&MusicItems,&MusicMenu[start],NULL);
        if (which>=0)
        {
            if (lastsong >= 0)
                MusicMenu[start+lastsong].active = 1;

            StartCPMusic(songs[start + which]);
            MusicMenu[start+which].active = 2;
            DrawMenu (&MusicItems,&MusicMenu[start]);
            VW_UpdateScreen();
            lastsong = which;
        }
    } while(which>=0);

    MenuFadeOut();
    IN_ClearKeysDown();
    UnCacheLump (CONTROLS_LUMP_START,CONTROLS_LUMP_END);
}

/*
==========================
=
= InitGame
=
= Load a few things right away
=
==========================
*/

static void InitGame()
{
    boolean didjukebox=false;

    // initialize SDL
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0)
    {
        printf("Unable to init SDL: %s\n", SDL_GetError());
        exit(1);
    }
    atexit(SDL_Quit);

    int numJoysticks = SDL_NumJoysticks();
    if(param_joystickindex && (param_joystickindex < -1 || param_joystickindex >= numJoysticks))
    {
        if(!numJoysticks)
            printf("No joysticks are available to SDL!\n");
        else
            printf("The joystick index must be between -1 and %i!\n", numJoysticks - 1);
        exit(1);
    }


    SignonScreen ();

	VW_UpdateScreen();

    VH_Startup ();
    IN_Startup ();
    PM_Startup ();
    SD_Startup ();
    CA_Startup ();
    US_Startup ();

    // TODO: Will any memory checking be needed someday??


//
// build some tables
//
    InitDigiMap ();

    ReadConfig ();

    SetupSaveGames();

//
// HOLDING DOWN 'M' KEY?
//
	IN_ProcessEvents();

    if (Keyboard[sc_M])
    {
        DoJukebox();
        didjukebox=true;
    }
    else

//
// draw intro screen stuff
//
    IntroScreen ();


//
// load in and lock down some basic chunks
//

    CA_CacheGrChunk(STARTFONT);
    CA_CacheGrChunk(STATUSBARPIC);

    LoadLatchMem ();
    BuildTables ();          // trig tables
    SetupWalls ();

    NewViewSize (viewsize);

//
// initialize variables
//
    InitRedShifts ();
    if(!didjukebox)
        FinishSignon();

}

//===========================================================================

/*
==========================
=
= SetViewSize
=
==========================
*/

boolean SetViewSize (unsigned width, unsigned height)
{
    viewwidth = width&~15;                  // must be divisable by 16
    viewheight = height&~1;                 // must be even
    centerx = viewwidth/2-1;
    shootdelta = viewwidth/10;
    if((unsigned) viewheight == screenHeight)
        viewscreenx = viewscreeny = screenofs = 0;
    else
    {
        viewscreenx = (screenWidth-viewwidth) / 2;
        viewscreeny = (screenHeight-scaleFactor*STATUSLINES-viewheight)/2;
        screenofs = viewscreeny*screenWidth+viewscreenx;
    }

//
// calculate trace angles and projection constants
//
    CalcProjection (FOCALLENGTH);

    return true;
}


void ShowViewSize (int width)
{
    int oldwidth,oldheight;

    oldwidth = viewwidth;
    oldheight = viewheight;

    if(width == 21)
    {
        viewwidth = screenWidth;
        viewheight = screenHeight;
        VWB_BarScaledCoord (0, 0, screenWidth, screenHeight, 0);
    }
    else if(width == 20)
    {
        viewwidth = screenWidth;
        viewheight = screenHeight - scaleFactor*STATUSLINES;
        DrawPlayBorder ();
    }
    else
    {
        viewwidth = width*16*screenWidth/320;
        viewheight = (int) (width*16*HEIGHTRATIO*screenHeight/200);
        DrawPlayBorder ();
    }

    viewwidth = oldwidth;
    viewheight = oldheight;
}


void NewViewSize (int width)
{
    viewsize = width;
    if(viewsize == 21)
        SetViewSize(screenWidth, screenHeight);
    else if(viewsize == 20)
        SetViewSize(screenWidth, screenHeight - scaleFactor * STATUSLINES);
    else
        SetViewSize(width*16*screenWidth/320, (unsigned) (width*16*HEIGHTRATIO*screenHeight/200));
}



//===========================================================================

/*
==========================
=
= Quit
=
==========================
*/

void Quit (const char *errorStr, ...)
{
    char error[256];
    if(errorStr != NULL)
    {
        va_list vlist;
        va_start(vlist, errorStr);
        vsprintf(error, errorStr, vlist);
        va_end(vlist);
    }
    else error[0] = 0;

    if (!pictable)  // don't try to display the red box before it's loaded
    {
        ShutdownId();
        if (*error)
        {
            puts(error);
            VW_WaitVBL(100);
        }
        exit(1);
    }

    if (!*error)
    {
        WriteConfig ();
    }

    ShutdownId ();

    if (*error)
    {
        puts(error);
        VW_WaitVBL(200);
        exit(1);
    }
    else
    if (!(*error))
    {
    }

    exit(0);
}

//===========================================================================



/*
=====================
=
= DemoLoop
=
=====================
*/


static void DemoLoop()
{
    int LastDemo = 0;

//
// check for launch from ted
//
    if (param_tedlevel != -1)
    {
        param_nowait = true;
        EnableEndGameMenuItem();
        NewGame(param_difficulty,0);

        gamestate.episode = param_tedlevel/10;
        gamestate.mapon = param_tedlevel%10;
        GameLoop();
        Quit (NULL);
    }


//
// main game cycle
//

#ifndef DEMOTEST

    #ifndef UPLOAD

        #ifndef GOODTIMES
        if (!param_nowait)
            NonShareware();
        #endif
    #endif

    StartCPMusic(INTROSONG);

    if (!param_nowait)
        PG13 ();

#endif

    while (1)
    {
        while (!param_nowait)
        {
//
// title page
//
#ifndef DEMOTEST

            CA_CacheScreen (TITLEPIC);
            VW_UpdateScreen ();
            VW_FadeIn();
            if (IN_UserInput(TickBase*15))
                break;
            VW_FadeOut();
//
// credits page
//
            CA_CacheScreen (CREDITSPIC);
            VW_UpdateScreen();
            VW_FadeIn ();
            if (IN_UserInput(TickBase*10))
                break;
            VW_FadeOut ();
//
// high scores
//
            DrawHighScores ();
            VW_UpdateScreen ();
            VW_FadeIn ();

            if (IN_UserInput(TickBase*10))
                break;
#endif
//
// demo
//

            PlayDemo (LastDemo++%4);

            if (playstate == ex_abort)
                break;
            VW_FadeOut();
            if(screenHeight % 200 != 0)
                VL_ClearScreen(0);
            StartCPMusic(INTROSONG);
        }

        VW_FadeOut ();

#ifdef DEBUGKEYS
        if (Keyboard[sc_Tab] && param_debugmode)
            RecordDemo ();
        else
            US_ControlPanel (0);
#else
        US_ControlPanel (0);
#endif

        if (startgame || loadedgame)
        {
            GameLoop ();
            if(!param_nowait)
            {
                VW_FadeOut();
                StartCPMusic(INTROSONG);
            }
        }
    }
}


//===========================================================================

#define IFARG(str) if(!strcmp(arg, (str)))

void CheckParameters(int argc, char *argv[])
{
    bool hasError = false, showHelp = false;
    bool sampleRateGiven = false, audioBufferGiven = false;
    int defaultSampleRate = param_samplerate;

    for(int i = 1; i < argc; i++)
    {
        char *arg = argv[i];
        IFARG("--goobers")
            param_debugmode = true;
        else IFARG("--baby")
            param_difficulty = 0;
        else IFARG("--easy")
            param_difficulty = 1;
        else IFARG("--normal")
            param_difficulty = 2;
        else IFARG("--hard")
            param_difficulty = 3;
        else IFARG("--nowait")
            param_nowait = true;
        else IFARG("--tedlevel")
        {
            if(++i >= argc)
            {
                printf("The tedlevel option is missing the level argument!\n");
                hasError = true;
            }
            else param_tedlevel = atoi(argv[i]);
        }
        else IFARG("--windowed")
            fullscreen = false;
        else IFARG("--windowed-mouse")
        {
            fullscreen = false;
            forcegrabmouse = true;
        }
        else IFARG("--res")
        {
            if(i + 2 >= argc)
            {
                printf("The res option needs the width and/or the height argument!\n");
                hasError = true;
            }
            else
            {
                screenWidth = atoi(argv[++i]);
                screenHeight = atoi(argv[++i]);
                unsigned factor = screenWidth / 320;
                if(screenWidth % 320 || (screenHeight != 200 * factor && screenHeight != 240 * factor))
                    printf("Screen size must be a multiple of 320x200 or 320x240!\n"), hasError = true;
            }
        }
        else IFARG("--resf")
        {
            if(i + 2 >= argc)
            {
                printf("The resf option needs the width and/or the height argument!\n");
                hasError = true;
            }
            else
            {
                screenWidth = atoi(argv[++i]);
                screenHeight = atoi(argv[++i]);
                if(screenWidth < 320)
                    printf("Screen width must be at least 320!\n"), hasError = true;
                if(screenHeight < 200)
                    printf("Screen height must be at least 200!\n"), hasError = true;
            }
        }
        else IFARG("--bits")
        {
            if(++i >= argc)
            {
                printf("The bits option is missing the color depth argument!\n");
                hasError = true;
            }
            else
            {
                screenBits = atoi(argv[i]);
                switch(screenBits)
                {
                    case 8:
                    case 16:
                    case 24:
                    case 32:
                        break;

                    default:
                        printf("Screen color depth must be 8, 16, 24, or 32!\n");
                        hasError = true;
                        break;
                }
            }
        }
        else IFARG("--nodblbuf")
            usedoublebuffering = false;
        else IFARG("--extravbls")
        {
            if(++i >= argc)
            {
                printf("The extravbls option is missing the vbls argument!\n");
                hasError = true;
            }
            else
            {
                extravbls = atoi(argv[i]);
                if(extravbls < 0)
                {
                    printf("Extravbls must be positive!\n");
                    hasError = true;
                }
            }
        }
        else IFARG("--joystick")
        {
            if(++i >= argc)
            {
                printf("The joystick option is missing the index argument!\n");
                hasError = true;
            }
            else param_joystickindex = atoi(argv[i]);   // index is checked in InitGame
        }
        else IFARG("--joystickhat")
        {
            if(++i >= argc)
            {
                printf("The joystickhat option is missing the index argument!\n");
                hasError = true;
            }
            else param_joystickhat = atoi(argv[i]);
        }
        else IFARG("--samplerate")
        {
            if(++i >= argc)
            {
                printf("The samplerate option is missing the rate argument!\n");
                hasError = true;
            }
            else param_samplerate = atoi(argv[i]);
            sampleRateGiven = true;
        }
        else IFARG("--audiobuffer")
        {
            if(++i >= argc)
            {
                printf("The audiobuffer option is missing the size argument!\n");
                hasError = true;
            }
            else param_audiobuffer = atoi(argv[i]);
            audioBufferGiven = true;
        }
        else IFARG("--mission")
        {
            if(++i >= argc)
            {
                printf("The mission option is missing the mission argument!\n");
                hasError = true;
            }
            else
            {
                param_mission = atoi(argv[i]);
                if(param_mission < 0 || param_mission > 3)
                {
                    printf("The mission option must be between 0 and 3!\n");
                    hasError = true;
                }
            }
        }
        else IFARG("--configdir")
        {
            if(++i >= argc)
            {
                printf("The configdir option is missing the dir argument!\n");
                hasError = true;
            }
            else
            {
                size_t len = strlen(argv[i]);
                if(len + 2 > sizeof(configdir))
                {
                    printf("The config directory is too long!\n");
                    hasError = true;
                }
                else
                {
                    strcpy(configdir, argv[i]);
                    if(argv[i][len] != '/' && argv[i][len] != '\\')
                        strcat(configdir, "/");
                }
            }
        }
        else IFARG("--goodtimes")
            param_goodtimes = true;
        else IFARG("--ignorenumchunks")
            param_ignorenumchunks = true;
        else IFARG("--help")
            showHelp = true;
        else hasError = true;
    }
    if(hasError || showHelp)
    {
        if(hasError) printf("\n");
        printf(
            "Wolf4SDL v1.7 ($Revision$)\n"
            "Ported by Chaos-Software (http://www.chaos-software.de.vu)\n"
            "Original Wolfenstein 3D by id Software\n\n"
            "Usage: Wolf4SDL [options]\n"
            "Options:\n"
            " --help                 This help page\n"
            " --tedlevel <level>     Starts the game in the given level\n"
            " --baby                 Sets the difficulty to baby for tedlevel\n"
            " --easy                 Sets the difficulty to easy for tedlevel\n"
            " --normal               Sets the difficulty to normal for tedlevel\n"
            " --hard                 Sets the difficulty to hard for tedlevel\n"
            " --nowait               Skips intro screens\n"
            " --windowed[-mouse]     Starts the game in a window [and grabs mouse]\n"
            " --res <width> <height> Sets the screen resolution\n"
            "                        (must be multiple of 320x200 or 320x240)\n"
            " --resf <w> <h>         Sets any screen resolution >= 320x200\n"
            "                        (which may result in graphic errors)\n"
            " --bits <b>             Sets the screen color depth\n"
            "                        (use this when you have palette/fading problems\n"
            "                        allowed: 8, 16, 24, 32, default: \"best\" depth)\n"
            " --nodblbuf             Don't use SDL's double buffering\n"
            " --extravbls <vbls>     Sets a delay after each frame, which may help to\n"
            "                        reduce flickering (unit is currently 8 ms, default: 0)\n"
            " --joystick <index>     Use the index-th joystick if available\n"
            "                        (-1 to disable joystick, default: 0)\n"
            " --joystickhat <index>  Enables movement with the given coolie hat\n"
            " --samplerate <rate>    Sets the sound sample rate (given in Hz, default: %i)\n"
            " --audiobuffer <size>   Sets the size of the audio buffer (-> sound latency)\n"
            "                        (given in bytes, default: 2048 / (44100 / samplerate))\n"
            " --ignorenumchunks      Ignores the number of chunks in VGAHEAD.*\n"
            "                        (may be useful for some broken mods)\n"
            " --configdir <dir>      Directory where config file and save games are stored\n"
            "                        (default: $HOME/.wolf4sdl)\n"
            , defaultSampleRate
        );
        exit(1);
    }

    if(sampleRateGiven && !audioBufferGiven)
        param_audiobuffer = 2048 / (44100 / param_samplerate);
}

/*
==========================
=
= main
=
==========================
*/

int main (int argc, char *argv[])
{
    CheckParameters(argc, argv);

    CheckForEpisodes();

    InitGame();

    DemoLoop();

    Quit("Demo loop exited???");
    return 1;
}
