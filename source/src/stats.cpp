
#include "logstats.hpp"
#include "cube.h"
#include <cassert>
//#include "server.h"

void stats_setupdemoplayback(const char* demopath=NULL);
void stats_readdemo();

static void stats_resetserver(const char *newname, int newmode, int newtime);
static void stats_startdemoplayback(const char *newname);

static void stats_servertoclient(int chan, uchar *buf, int len, bool demo);
static void stats_sendpacket(int n, int chan, ENetPacket *packet, int exclude, bool demopacket);
static void stats_sendservmsg(const char *msg, int cn = -1);
static void stats_sendf(int cn, int chan, const char *format, ...);
static void stats_enddemoplayback();

static void stats_parsepositions(ucharbuf &p);
static void stats_parsemessages(int cn, playerent *d, ucharbuf &p, bool demo = false);
static playerent *stats_getclient(int cn);
static playerent *stats_newclient(int cn);
static int stats_getclientnum();
static playerent *stats_newplayerent();

static stream *demotmp = NULL, *demorecord = NULL, *demoplayback = NULL;
static int nextplayback = 0;
static int minremain = 0, gamemillis = 0, gamelimit = 0, /*lmsitemtype = 0,*/ nextsendscore = 0;
static int smode = 0, nextgamemode;
static int arenaround = 0, arenaroundstartmillis = 0;
static mapstats smapstats;

static bool stats_watchingdemo = false;
static int stats_demoprotocol = 0;
static int stats_lastmillis = 0;


static string stats_smapname;

static playerent *stats_player1 = stats_newplayerent();          // our client
static vector<playerent *> stats_players;                  // other clients

//FIXME: should this be true or false
static bool stats_modprotocol = true;
#define STATS_CUR_PROTOCOL_VERSION (stats_modprotocol ? -PROTOCOL_VERSION : PROTOCOL_VERSION)



void rstats();



playerent *stats_newplayerent()                 // create a new blank player
{
    playerent *d = new playerent;
    //d->lastupdate = totalmillis;
    //setskin(d, rnd(6));
    //weapon::equipplayer(d); // flowtron : avoid overwriting d->spawnstate(gamemode) stuff from the following line (this used to be called afterwards)
    //spawnstate(d);
    return d;
}


const char* event_to_str(int type)
{
    switch (type){
        case (SV_SERVINFO): return "SV_SERVINFO";
        case (SV_WELCOME): return "SV_WELCOME";
        case (SV_INITCLIENT): return "SV_INITCLIENT";
        case (SV_POS): return "SV_POS";
        case (SV_POSC): return "SV_POSC";
        case (SV_POSN): return "SV_POSN";
        case (SV_TEXT): return "SV_TEXT";
        case (SV_TEAMTEXT): return "SV_TEAMTEXT";
        case (SV_TEXTME): return "SV_TEXTME";
        case (SV_TEAMTEXTME): return "SV_TEAMTEXTME";
        case (SV_TEXTPRIVATE): return "SV_TEXTPRIVATE";
        
        case (SV_SOUND): return "SV_SOUND";
        case (SV_VOICECOM): return "SV_VOICECOM";
        case (SV_VOICECOMTEAM): return "SV_VOICECOMTEAM";
        case (SV_CDIS): return "SV_CDIS";
        
        case (SV_SHOOT): return "SV_SHOOT";
        case (SV_EXPLODE): return "SV_EXPLODE";
        case (SV_SUICIDE): return "SV_SUICIDE";
        case (SV_AKIMBO): return "SV_AKIMBO";
        case (SV_RELOAD): return "SV_RELOAD";
        case (SV_AUTHT): return "SV_AUTHT";
        case (SV_AUTHREQ): return "SV_AUTHREQ";
        case (SV_AUTHTRY): return "SV_AUTHTRY";
        case (SV_AUTHANS): return "SV_AUTHANS";
        case (SV_AUTHCHAL): return "SV_AUTHCHAL";
        
        case (SV_GIBDIED): return "SV_GIBDIED";
        case (SV_DIED): return "SV_DIED";
        case (SV_GIBDAMAGE): return "SV_GIBDAMAGE";
        case (SV_DAMAGE): return "SV_DAMAGE";
        case (SV_HITPUSH): return "SV_HITPUSH";
        case (SV_SHOTFX): return "SV_SHOTFX";
        case (SV_THROWNADE): return "SV_THROWNADE";
        
        
        case (SV_TRYSPAWN): return "SV_TRYSPAWN";
        case (SV_SPAWNSTATE): return "SV_SPAWNSTATE";
        case (SV_SPAWN): return "SV_SPAWN";
        case (SV_SPAWNDENY): return "SV_SPAWNDENY";
        case (SV_FORCEDEATH): return "SV_FORCEDEATH";
        case (SV_RESUME): return "SV_RESUME";
        
        
        case (SV_DISCSCORES): return "SV_DISCSCORES";
        case (SV_TIMEUP): return "SV_TIMEUP";
        case (SV_EDITENT): return "SV_EDITENT";
        case (SV_ITEMACC): return "SV_ITEMACC";
        
        
        case (SV_MAPCHANGE): return "SV_MAPCHANGE";
        case (SV_ITEMSPAWN): return "SV_ITEMSPAWN";
        case (SV_ITEMPICKUP): return "SV_ITEMPICKUP";
        
        case (SV_PING): return "SV_PING";
        case (SV_PONG): return "SV_PONG";
        case (SV_CLIENTPING): return "SV_CLIENTPING";
        case (SV_GAMEMODE): return "SV_GAMEMODE";
        
        case (SV_EDITMODE): return "SV_EDITMODE";
        case (SV_EDITH): return "SV_EDITH";
        case (SV_EDITT): return "SV_EDITT";
        case (SV_EDITS): return "SV_EDITS";
        case (SV_EDITD): return "SV_EDITD";
        case (SV_EDITE): return "SV_EDITE";
        case (SV_NEWMAP): return "SV_NEWMAP";
        
        case (SV_SENDMAP): return "SV_SENDMAP";
        case (SV_RECVMAP): return "SV_RECVMAP";
        case (SV_REMOVEMAP): return "SV_REMOVEMAP";
        
        case (SV_SERVMSG): return "SV_SERVMSG";
        case (SV_ITEMLIST): return "SV_ITEMLIST";
        case (SV_WEAPCHANGE): return "SV_WEAPCHANGE";
        case (SV_PRIMARYWEAP): return "SV_PRIMARYWEAP";
        
        case (SV_FLAGACTION): return "SV_FLAGACTION";
        case (SV_FLAGINFO): return "SV_FLAGINFO";
        case (SV_FLAGMSG): return "SV_FLAGMSG";
        case (SV_FLAGCNT): return "SV_FLAGCNT";
        
        case (SV_ARENAWIN): return "SV_ARENAWIN";
        
        case (SV_SETADMIN): return "SV_SETADMIN";
        case (SV_SERVOPINFO): return "SV_SERVOPINFO";
        
        case (SV_CALLVOTE): return "SV_CALLVOTE";
        case (SV_CALLVOTESUC): return "SV_CALLVOTESUC";
        case (SV_CALLVOTEERR): return "SV_CALLVOTEERR";
        case (SV_VOTE): return "SV_VOTE";
        case (SV_VOTERESULT): return "SV_VOTERESULT";
        
        case (SV_SETTEAM): return "SV_SETTEAM";
        case (SV_TEAMDENY): return "SV_TEAMDENY";
        case (SV_SERVERMODE): return "SV_SERVERMODE";
        
        case (SV_IPLIST): return "SV_IPLIST";
        
        case (SV_LISTDEMOS): return "SV_LISTDEMOS";
        case (SV_SENDDEMOLIST): return "SV_SENDDEMOLIST";
        case (SV_GETDEMO): return "SV_GETDEMO";
        case (SV_SENDDEMO): return "SV_SENDDEMO";
        case (SV_DEMOPLAYBACK): return "SV_DEMOPLAYBACK";
        
        case (SV_CONNECT): return "SV_CONNECT";
        
        case (SV_SWITCHNAME): return "SV_SWITCHNAME";
        case (SV_SWITCHSKIN): return "SV_SWITCHSKIN";
        case (SV_SWITCHTEAM): return "SV_SWITCHTEAM";
        
        case (SV_CLIENT): return "SV_CLIENT";
        
        case (SV_EXTENSION): return "SV_EXTENSION";
        
        case (SV_MAPIDENT): return "SV_MAPIDENT";
        case (SV_HUDEXTRAS): return "SV_HUDEXTRAS";
        case (SV_POINTS): return "SV_POINTS";
        
        case (SV_NUM): return "SV_NUM";
        
        default: return "UKNOWNEVENT";
        
    }
    
    /*
    SV_CONNECT,
    SV_SWITCHNAME, SV_SWITCHSKIN, SV_SWITCHTEAM,
    SV_CLIENT,
    SV_EXTENSION,
    SV_MAPIDENT, SV_HUDEXTRAS, SV_POINTS,
    SV_NUM
    */
    
    
}


void stats_resetserver(const char *newname, int newmode, int newtime)
{

    smode = newmode;
    copystring(stats_smapname, newname);

    minremain = newtime > 0 ? newtime : defaultgamelimit(newmode);
    gamemillis = 0;
    gamelimit = minremain*60000;
    arenaround = arenaroundstartmillis = 0;
    memset(&smapstats, 0, sizeof(smapstats));

    /*
    interm = nextsendscore = 0;
    lastfillup = servmillis;
    sents.shrink(0);
    if(mastermode == MM_PRIVATE)
    {
        loopv(savedscores) savedscores[i].valid = false;
    }
    else savedscores.shrink(0);
    ctfreset();

    nextmapname[0] = '\0';
    forceintermission = false;
    */
}


void stats_startdemoplayback(const char *newname)
{
    //if(isdedicated) return;
    stats_resetserver(newname, GMODE_DEMO, -1);
    stats_setupdemoplayback();
}

void stats_enddemoplayback()
{
    if(!demoplayback) return;
    delete demoplayback;
    demoplayback = NULL;
    stats_watchingdemo = false;

    //loopv(clients) stats_sendf(i, 1, "risi", SV_DEMOPLAYBACK, "", i);

    stats_sendservmsg("demo playback finished");

    //loopv(clients) sendwelcome(clients[i]);
}

void stats_setupdemoplayback(const char* demopath)
{
    demoheader hdr;
    string msg;
    msg[0] = '\0';
    
    const char* file_path_ptr = NULL;
    if (demopath) {
        
        demoplayback = opengzfile(demopath, "rb");
        file_path_ptr = demopath;
    } else {
        
        defformatstring(file)("demos/%s.dmo", stats_smapname);
        
        path(file);
        demoplayback = opengzfile(file, "rb");
        
        file_path_ptr = file;
    }
    
    if(!demoplayback) {
        formatstring(msg)("could not read demo \"%s\"", file_path_ptr);
    } else if(demoplayback->read(&hdr, sizeof(demoheader))!=sizeof(demoheader) || memcmp(hdr.magic, DEMO_MAGIC, sizeof(hdr.magic)))
        formatstring(msg)("\"%s\" is not a demo file", file_path_ptr);
    else
    {
        lilswap(&hdr.version, 1);
        lilswap(&hdr.protocol, 1);
        if(hdr.version!=DEMO_VERSION) formatstring(msg)("demo \"%s\" requires an %s version of AssaultCube", file_path_ptr, hdr.version<DEMO_VERSION ? "older" : "newer");
        else if(hdr.protocol != PROTOCOL_VERSION && !(hdr.protocol < 0 && hdr.protocol == -PROTOCOL_VERSION) && hdr.protocol != 1132) formatstring(msg)("demo \"%s\" requires an %s version of AssaultCube", file_path_ptr, hdr.protocol<PROTOCOL_VERSION ? "older" : "newer");
        else if(hdr.protocol == 1132) stats_sendservmsg("WARNING: using experimental compatibility mode for older demo protocol, expect breakage");
        stats_demoprotocol = hdr.protocol;
    }
    if(msg[0])
    {
        if(demoplayback) { delete demoplayback; demoplayback = NULL; }
        stats_sendservmsg(msg);
        return;
    }

    formatstring(msg)("playing demo \"%s\"", file_path_ptr);
    stats_sendservmsg(msg);
    stats_sendf(-1, 1, "risi", SV_DEMOPLAYBACK, stats_smapname, -1);
    stats_watchingdemo = true;

    if(demoplayback->read(&nextplayback, sizeof(nextplayback))!=sizeof(nextplayback))
    {
        stats_enddemoplayback();
        return;
    }
    lilswap(&nextplayback, 1);
}


void stats_readdemo()
{
    if(!demoplayback) return;
    while(true)
    {
        //logstats("stats_readdemo.while.1", {{"nextplayback",nextplayback},{"gamemillis",gamemillis}});
        
        {
            dict_t outdata("nextplayback");
            outdata.set("typestr", "nextplayback");
            outdata.set("nextplayback", nextplayback);
        }
        
        int chan, len;
        if(demoplayback->read(&chan, sizeof(chan))!=sizeof(chan) ||
           demoplayback->read(&len, sizeof(len))!=sizeof(len))
        {
            stats_enddemoplayback();
            //logstats("stats_readdemo.return.1", {});
            return;
        }
        lilswap(&chan, 1);
        lilswap(&len, 1);
        ENetPacket *packet = enet_packet_create(NULL, len, 0);
        if(!packet || demoplayback->read(packet->data, len)!=len)
        {
            if(packet) enet_packet_destroy(packet);
            stats_enddemoplayback();
            //logstats("stats_readdemo.return.2", {});
            return;
        }
        
        
        
        stats_sendpacket(-1, chan, packet, -1, true);
        if(!packet->referenceCount) enet_packet_destroy(packet);
        if(demoplayback->read(&nextplayback, sizeof(nextplayback))!=sizeof(nextplayback))
        {
            stats_enddemoplayback();
            //logstats("stats_readdemo.return.3", {});
            return;
        }
        lilswap(&nextplayback, 1);
    }
}



static void stats_sendf(int cn, int chan, const char *format, ...)
{
    int exclude = -1;
    bool reliable = false;
    if(*format=='r') { reliable = true; ++format; }
    packetbuf p(MAXTRANS, reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    va_list args;
    va_start(args, format);
    while(*format) switch(*format++)
    {
        case 'x':
            exclude = va_arg(args, int);
            break;

        case 'v':
        {
            int n = va_arg(args, int);
            int *v = va_arg(args, int *);
            loopi(n) putint(p, v[i]);
            break;
        }

        case 'i':
        {
            int n = isdigit(*format) ? *format++-'0' : 1;
            loopi(n) putint(p, va_arg(args, int));
            break;
        }
        case 's': sendstring(va_arg(args, const char *), p); break;
        case 'm':
        {
            int n = va_arg(args, int);
            p.put(va_arg(args, uchar *), n);
            break;
        }
    }
    va_end(args);
    ///fixme: should demo be true? default was blank
    stats_sendpacket(cn, chan, p.finalize(), exclude, true /*demo*/);
}


static void stats_sendservmsg(const char *msg, int cn)
{
    stats_sendf(cn, 1, "ris", SV_SERVMSG, msg);
}


void stats_sendpacket(int n, int chan, ENetPacket *packet, int exclude, bool demopacket)
{
    ///broadcast to all clients
    if(n<0)
    {
    //    recordpacket(chan, packet->data, (int)packet->dataLength);
    //    loopv(clients) if(i!=exclude && (clients[i]->type!=ST_TCPIP || clients[i]->isauthed)) sendpacket(i, chan, packet, -1, demopacket);
    // return;
    }
    ///particular client
    stats_servertoclient(chan, packet->data, (int)packet->dataLength, demopacket);
}


void stats_servertoclient(int chan, uchar *buf, int len, bool demo)   // processes any updates from the server
{
    
    //logdata_t logdata{{"chan",chan}, {"len",len}, {"demo",demo}};
    //logstats("stats_servertoclient.begin", {}, logdata);

    ucharbuf p(buf, len);
    switch(chan)
    {
        ///todo
        //case 0: stats_parsepositions(p); break;
        case 1: {
            stats_parsemessages(-1, NULL, p, demo);
            
            if (p.remaining() != 0)
            {
                //logstats("stats_servertoclient.WARNING", {{"p.remaining()",p.remaining()}}, logdata);
            }
            break;
        }
        ///todo
        //case 2: receivefile(p.buf, p.maxlen); break;
    }
    
}

#if 0
void stats_parsepositions(ucharbuf &p)
{
    int type;
    while(p.remaining()) switch(type = getint(p))
    {
        case SV_POS:                        // position of another client
        case SV_POSC:
        {
            int cn, f, g;
            vec o, vel;
            
            float yaw, pitch, roll = 0;
            bool scoping;//, shoot;
            if(type == SV_POSC)
            {
                bitbuf<ucharbuf> q(p);
                cn = q.getbits(5);
                int usefactor = q.getbits(2) + 7;
                
                o.x = (q.getbits(usefactor + 4)) / DMF;
                o.y = (q.getbits(usefactor + 4)) / DMF;
                yaw = q.getbits(9) * 360.0f / 512;
                pitch = (q.getbits(8) - 128) * 90.0f / 127;
                roll = !q.getbits(1) ? (q.getbits(6) - 32) * 20.0f / 31 : 0.0f;
                if(!q.getbits(1))
                {
                    vel.x = (q.getbits(4) - 8) / DVELF;
                    vel.y = (q.getbits(4) - 8) / DVELF;
                    vel.z = (q.getbits(4) - 8) / DVELF;
                }
                else vel.x = vel.y = vel.z = 0.0f;
                f = q.getbits(8);
                int negz = q.getbits(1);
                int full = q.getbits(1);
                int s = q.rembits();
                if(s < 3) s += 8;
                if(full) s = 11;
                int z = q.getbits(s);
                if(negz) z = -z;
                o.z = z / DMF;
                scoping = ( q.getbits(1) ? true : false );
                q.getbits(1);//shoot = ( q.getbits(1) ? true : false );
            }
            else
            {
                cn = getint(p);
                o.x   = getuint(p)/DMF;
                o.y   = getuint(p)/DMF;
                o.z   = getuint(p)/DMF;
                yaw   = (float)getuint(p);
                pitch = (float)getint(p);
                g = getuint(p);
                if ((g>>3) & 1) roll  = (float)(getint(p)*20.0f/125.0f);
                if (g & 1) vel.x = getint(p)/DVELF; else vel.x = 0;
                if ((g>>1) & 1) vel.y = getint(p)/DVELF; else vel.y = 0;
                if ((g>>2) & 1) vel.z = getint(p)/DVELF; else vel.z = 0;
                scoping = ( (g>>4) & 1 ? true : false );
                //shoot = ( (g>>5) & 1 ? true : false ); // we are not using this yet
                f = getuint(p);
            }
            int seqcolor = (f>>6)&1;
            playerent *d = stats_getclient(cn);
            if(!d || seqcolor!=(d->lifesequence&1)) continue;
            vec oldpos(d->o);
            float oldyaw = d->yaw, oldpitch = d->pitch;
            loopi(3)
            {
                float dr = o.v[i] - d->o.v[i] + ( i == 2 ? d->eyeheight : 0);
                if ( !dr ) d->vel.v[i] = 0.0f;
                else if ( d->vel.v[i] ) d->vel.v[i] = dr * 0.05f + d->vel.v[i] * 0.95f;
                d->vel.v[i] += vel.v[i];
                if ( i==2 && d->onfloor && d->vel.v[i] < 0.0f ) d->vel.v[i] = 0.0f;
            }
            d->o = o;
            d->o.z += d->eyeheight;
            d->yaw = yaw;
            d->pitch = pitch;
            //if(d->weaponsel->type == GUN_SNIPER)
            //{
            //    sniperrifle *sr = (sniperrifle *)d->weaponsel;
            //    sr->scoped = d->scoping = scoping;
            //}
            d->roll = roll;
            d->strafe = (f&3)==3 ? -1 : f&3;
            f >>= 2;
            d->move = (f&3)==3 ? -1 : f&3;
            f >>= 2;
            d->onfloor = f&1;
            f >>= 1;
            d->onladder = f&1;
            f >>= 2;
            //d->last_pos = totalmillis;
            //updatecrouch(d, f&1);
            //updatepos(d);
            //updatelagtime(d);
            /*
            logdata_t event_logdata {{"cn", cn},{"type", type},{"typestr", event_to_str(type)}};

            logstats( type == SV_POS ? "SV_POS" : "SV_POSC"
                    , {   {"cn", cn}
                        , {"yaw",yaw}
                        , {"pitch", pitch}
                        , {"roll", roll}
                        , {"scoping", scoping}
                        , {"o.x", o.x}
                        , {"o.y", o.y}
                        , {"o.z", o.z}
                        , {"vel.x", vel.x}
                        , {"vel.y", vel.y}
                        , {"vel.z", vel.z} }
                    , event_logdata);

            */

            break;
        }

        default:
            neterr("type");
            return;
    }
}

#endif



void stats_parsemessages(int cn, playerent *d, ucharbuf &p, bool demo)
{
    //logstats("packet", {{"cn", cn}});
    
    int cn0 = cn;
    
    static char text[MAXTRANS];
    int type, joining = 0;
    bool demoplayback = false;

    while(p.remaining())
    {
        type = getint(p);

        //logdata_t event_logdata {{"cn", cn},{"type", type},{"typestr", event_to_str(type)}};
        //logstats("event", event_logdata);
        

        if(demo && stats_watchingdemo && stats_demoprotocol == 1132)
        {
            if(type > SV_IPLIST) --type;            // SV_WHOIS removed
            if(type >= SV_TEXTPRIVATE) ++type;      // SV_TEXTPRIVATE added
            if(type == SV_SWITCHNAME)               // SV_SPECTCN removed
            {
                getint(p);
                continue;
            }
            else if(type > SV_SWITCHNAME) --type;
        }
        
        
        #ifdef _DEBUG
        if(type!=SV_POS && type!=SV_CLIENTPING && type!=SV_PING && type!=SV_PONG && type!=SV_CLIENT)
        {
            DEBUGVAR(d);
            ASSERT(type>=0 && type<SV_NUM);
            DEBUGVAR(messagenames[type]);
            protocoldebug(DEBUGCOND);
        }
        else protocoldebug(false);
        #endif

        switch(type)
        {
            case SV_SERVINFO:  // welcome message from the server
            {
                
                int mycn = getint(p), prot = getint(p);
                if(prot!=STATS_CUR_PROTOCOL_VERSION && !(stats_watchingdemo && prot == -PROTOCOL_VERSION))
                {
                    conoutf(_("%c3incompatible game protocol (local protocol: %d :: server protocol: %d)"), CC, STATS_CUR_PROTOCOL_VERSION, prot);
                    conoutf("\f3if this occurs a lot, obtain an upgrade from \f1http://assault.cubers.net");
                    if(stats_watchingdemo) conoutf("breaking loop : \f3this demo is using a different protocol\f5 : end it now!"); // SVN-WiP-bug: causes endless retry loop else!
                    //else disconnect();
                    
                    ///FIXME: prolly want to throw an error or something here
                    
                    dict_t outdata(event_to_str(type));
                    outdata.set("cn", cn0);
                    outdata.set("typestr", event_to_str(type));
                    outdata.set("prot", prot);
                    outdata.set("CUR_PROTOCOL_VERSION",STATS_CUR_PROTOCOL_VERSION);
                    outdata.set("mycn",mycn);

                    return;
                }
                int sessionid = getint(p);
                stats_player1->clientnum = mycn;
                if(getint(p) > 0) conoutf(_("INFO: this server is password protected"));
                break;
            }
            case SV_WELCOME:
            {
                joining = getint(p);
                //player1->resetspec();
                //resetcamera();

                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("joining", joining);
                break;
            }
            case SV_CLIENT:
            {
                int cn = getint(p), len = getuint(p);
                ucharbuf q = p.subbuf(len);
                stats_parsemessages(cn, stats_getclient(cn), q, demo);
                break;
            }
            case SV_SOUND:
            {
                int sound = getint(p);

                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("sound", sound);
                //audiomgr.playsound(sound, d);
                break;
            }
            case SV_VOICECOMTEAM:
            {
                playerent *d = stats_getclient(getint(p));
                //if(d) d->lastvoicecom = lastmillis;
                int t = getint(p);
                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("t", t);
                
                if(!d || !(d->muted || d->ignored))
                {
                    //if ( voicecomsounds == 1 || (voicecomsounds == 2 && m_teammode) ) audiomgr.playsound(t, SP_HIGH);
                }
                break;
            }
            case SV_VOICECOM:
            {
                int t = getint(p);
                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("t", t);
                
                if(!d || !(d->muted || d->ignored))
                {
                    //if ( voicecomsounds == 1 ) audiomgr.playsound(t, SP_HIGH);
                }
                //if(d) d->lastvoicecom = lastmillis;
                break;
            }

            case SV_TEAMTEXTME:
            case SV_TEAMTEXT:
            {
                int cn = getint(p);
                getstring(text, p);

                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("text", (const char*)text);
                

                break;
            }

            case SV_TEXTME:
            case SV_TEXT:
            {
                getstring(text, p);
                
                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("text",(const char*)text);
                
                break;
            }
            case SV_TEXTPRIVATE:
            {
                int cn = getint(p);
                getstring(text, p);
                filtertext(text, text, FTXT__CHAT);
                playerent *d = stats_getclient(cn);
                if(!d) break;
                //if(d->ignored) clientlogf("ignored: pm %s %s", colorname(d), text);
                //else
                {
                    //conoutf("%s (PM):\f9 %s", colorname(d), highlight(text));
                    //lastpm = d->clientnum;
                    //if(identexists("onPM"))
                    {
                        //defformatstring(onpm)("onPM %d [%s]", d->clientnum, text);
                        //execute(onpm);
                    }
                }
                break;
            }

            case SV_MAPCHANGE:
            {
                //spawnpermission = SP_SPECT;
                getstring(text, p);
                int mode = getint(p);
                int downloadable = getint(p);
                int revision = getint(p);
                //localwrongmap = !changemapserv(text, mode, downloadable, revision);
                //if(m_arena && joining>2) deathstate(player1);
                
                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("revision",revision);
                outdata.set("mode",mode);
                outdata.set("name",(const char*)text);
                break;
            }

            case SV_ITEMLIST:
            {
                int n;
                //resetspawns();
                while((n = getint(p))!=-1)
                {
                    //setspawn(n, true);
                }
                break;
            }

            case SV_MAPIDENT:
            {
                int unknown_ints[2];
                loopi(2) unknown_ints[i] = getint(p);
                

                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("unknown_ints[0]", unknown_ints[0]);
                outdata.set("unknown_ints[1]", unknown_ints[1]);
                
                break;
            }

            case SV_SWITCHNAME:
            {
                getstring(text, p);
                filtertext(text, text, FTXT__PLAYERNAME, MAXNAMELEN);
                if(!text[0]) copystring(text, "unarmed");
                if(d)
                {
                    //if(strcmp(d->name, text))
                        //conoutf(_("%s is now known as %s"), colorname(d), colorname(d, text));
                    if(identexists("onNameChange"))
                    {
                        //defformatstring(onnamechange)("onNameChange %d \"%s\"", d->clientnum, text);
                        //execute(onnamechange);
                    }
                    copystring(d->name, text, MAXNAMELEN+1);
                    //updateclientname(d);
                }
                
                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("name", (const char*)text);
                break;
            }
            case SV_SWITCHTEAM:
            {
                int unknown_int = getint(p);

                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("unknown_int", unknown_int);
                break;
            }
            case SV_SWITCHSKIN:
            {
                loopi(2)
                {
                    int skin = getint(p);
                    //if(d) d->setskin(i, skin);
                }
                break;
            }
            
            
            
            case SV_INITCLIENT:            // another client either connected or changed name/team
            {
                int cn = getint(p);
                playerent *d = stats_newclient(cn);
                if(!d)
                {
                    getstring(text, p);
                    loopi(2) int skin = getint(p);
                    int team = getint(p);
                    int address = 0;
                    if(!demo || !stats_watchingdemo || stats_demoprotocol > 1132)
                    {
                        getint(p);
                    }
                    
                    dict_t outdata(event_to_str(type));
                    outdata.set("cn", cn0);
                    outdata.set("typestr", event_to_str(type));
                    outdata.set("ccn",cn);
                    outdata.set("name",(const char*)text);
                    outdata.set("team",team);
                    outdata.set("pip",address);
                    outdata.set("success",0);

                    break;
                }
                getstring(text, p);
                filtertext(text, text, FTXT__PLAYERNAME, MAXNAMELEN);
                if(!text[0]) copystring(text, "unarmed");
                if(d->name[0])          // already connected
                {
                    //if(strcmp(d->name, text))
                    //    conoutf(_("%s is now known as %s"), colorname(d), colorname(d, text));
                }
                else                    // new client
                {
                    //conoutf(_("connected: %s"), colorname(d, text));
                }
                copystring(d->name, text, MAXNAMELEN+1);
                if(identexists("onConnect"))
                {
                    //defformatstring(onconnect)("onConnect %d", d->clientnum);
                    //execute(onconnect);
                }
                loopi(2) d->setskin(i, getint(p));
                d->team = getint(p);

                int address = 0;
                if(!demo || !stats_watchingdemo || stats_demoprotocol > 1132){
                    address = getint(p); // partial IP address
                    d->address = address;
                    
                }
            
                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("ccn",cn);
                outdata.set("name",(const char*)text);
                outdata.set("team",d->team);
                outdata.set("pip",address);
                outdata.set("success",1);
                /*
                if(m_flags) loopi(2)
                {
                    flaginfo &f = flaginfos[i];
                    if(!f.actor) f.actor = stats_getclient(f.actor_cn);
                }
                */
                //updateclientname(d);
                break;
            }

            case SV_CDIS:
            {
                int cn = getint(p);
                playerent *d = stats_getclient(cn);
                
                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("ccn",cn);
                
                if(!d) break;
                //if(d->name[0]) conoutf(_("player %s disconnected"), colorname(d));
                //stats_zapplayer(players[cn]);
                if(identexists("onDisconnect"))
                {
                    //defformatstring(ondisconnect)("onDisconnect %d", d->clientnum);
                    //execute(ondisconnect);
                }
                break;
            }

            case SV_EDITMODE:
            {
                int val = getint(p);
                if(!d) break;
                if(val) d->state = CS_EDITING;
                else
                {
                    //2011oct16:flowtron:keep spectator state
                    //specators shouldn't be allowed to toggle editmode for themselves. they're ghosts!
                    d->state = d->state==CS_SPECTATE?CS_SPECTATE:CS_ALIVE;
                }
                break;
            }



            case SV_SPAWN:
            {
                playerent *s = d;
                if(!s) { static playerent dummy; s = &dummy; }
                //s->respawn();
                s->lifesequence = getint(p);
                s->health = getint(p);
                s->armour = getint(p);
                int gunselect = getint(p);
                //s->setprimary(gunselect);
                //s->selectweapon(gunselect);
                loopi(NUMGUNS) s->ammo[i] = getint(p);
                loopi(NUMGUNS) s->mag[i] = getint(p);
                s->state = CS_SPAWNING;
                //if(s->lifesequence==0) s->resetstats(); //NEW

                ///FIXME: dump s->ammo and s->mag
                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("lifesequence",s->lifesequence);
                outdata.set("health",s->health);
                outdata.set("armour",s->armour);
                outdata.set("gunselect",s->gunselect);
                break;
            }
            case SV_SPAWNSTATE:
            {
                //if ( map_quality == MAP_IS_BAD )
                //{
                //    loopi(6+2*NUMGUNS) getint(p);
                //    conoutf(_("map deemed unplayable - fix it before you can spawn"));
                //    break;
                //}

                //if(editmode) toggleedit(true);
                //showscores(false);
                //setscope(false);
                //setburst(false);
                //stats_player1->respawn();
                int lifesequence = getint(p);
                int health = getint(p);
                int armour = getint(p);
                int setprimary(getint(p));
                int selectweapon(getint(p));
                int arenaspawn = getint(p);
                loopi(NUMGUNS) int ammoi = getint(p);
                loopi(NUMGUNS) int magi = getint(p);
                int state = CS_ALIVE;
                //lastspawn = lastmillis;
                //findplayerstart(player1, false, arenaspawn);
                //arenaintermission = 0;
                //if(m_arena && !localwrongmap)
                //{
                    //if(connected) closemenu(NULL);
                    //conoutf(_("new round starting... fight!"));
                    //hudeditf(HUDMSG_TIMER, "FIGHT!");
                    //if(m_botmode) BotManager.RespawnBots();
                //}
                //addmsg(SV_SPAWN, "rii", stats_player1->lifesequence, player1->weaponsel->type);
                //stats_player1->weaponswitch(stats_player1->primweap);
                //stats_player1->weaponchanging -= weapon::weaponchangetime/2;
                //if(stats_player1->lifesequence==0) stats_player1->resetstats(); //NEW

                ///FIXME: dump ammoi and magi
                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("lifesequence",lifesequence);
                outdata.set("health",health);
                outdata.set("armour",armour);
                outdata.set("setprimary",setprimary);
                outdata.set("selectweapon",selectweapon);
                outdata.set("arenaspawn",arenaspawn);
 
                break;
            }

            case SV_SHOTFX:
            {
                int scn = getint(p), gun = getint(p);
                vec from, to;
                loopk(3) to[k] = getint(p)/DMF;
                playerent *s = stats_getclient(scn);
                
                
                if(!s || !weapon::valid(gun))
                {
                    dict_t outdata(event_to_str(type));
                    outdata.set("cn", cn0);
                    outdata.set("typestr", event_to_str(type));
                    outdata.set("to.x", to.x);
                    outdata.set("to.y", to.y);
                    outdata.set("to.z", to.z);
                    outdata.set("shooter", scn);
                    outdata.set("gun", gun);
                
                    break;
                }
                loopk(3) from[k] = s->o.v[k];
                
                
                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("from.x",from.x);
                outdata.set("from.y",from.y);
                outdata.set("from.z",from.z);
                outdata.set("to.x",to.x);
                outdata.set("to.y",to.y);
                outdata.set("to.z",to.z);
                outdata.set("shooter",scn);
                outdata.set("gun",gun);
                
                    
                //if(gun==GUN_SHOTGUN) createrays(from, to);
                //s->lastaction = lastmillis;
                //s->weaponchanging = 0;
                //s->mag[gun]--;
                //if(s->weapons[gun])
                //{
                //    s->lastattackweapon = s->weapons[gun];
                //    s->weapons[gun]->gunwait = s->weapons[gun]->info.attackdelay;
                //    s->weapons[gun]->attackfx(from, to, -1);
                //    s->weapons[gun]->reloading = 0;
                //}
                //s->pstatshots[gun]++; //NEW
                break;
            }

            case SV_THROWNADE:
            {
                vec from, to;
                loopk(3) from[k] = getint(p)/DMF;
                loopk(3) to[k] = getint(p)/DMF;
                int nademillis = getint(p);
                
                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("from.x",from.x);
                outdata.set("from.y",from.y);
                outdata.set("from.z",from.z);
                outdata.set("to.x",to.x);
                outdata.set("to.y",to.y);
                outdata.set("to.z",to.z);
                outdata.set("nademillis",nademillis);
                
                if(!d) break;
                d->lastaction = stats_lastmillis;
                d->weaponchanging = 0;
                d->lastattackweapon = d->weapons[GUN_GRENADE];
                //if(d->weapons[GUN_GRENADE])
                //{
                //    d->weapons[GUN_GRENADE]->attackfx(from, to, nademillis);
                //    d->weapons[GUN_GRENADE]->reloading = 0;
                //}
                if(d!=stats_player1) d->pstatshots[GUN_GRENADE]++; //NEW
                break;
            }

            case SV_RELOAD:
            {
                int cn = getint(p), gun = getint(p);
                playerent *p = stats_getclient(cn);
                //if(p && p!=player1) p->weapons[gun]->reload(false);

                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("ccn", cn);
                outdata.set("gun", gun);

                break;
            }

            // for AUTH: WIP

            case SV_AUTHREQ:
            {
                //extern int autoauth;
                getstring(text, p);
                //if(autoauth && text[0] && tryauth(text)) conoutf("server requested authkey \"%s\"", text);
                break;
            }

            case SV_AUTHCHAL:
            {
                getstring(text, p);
                authkey *a = findauthkey(text);
                uint id = (uint)getint(p);
                getstring(text, p);
                if(a && a->lastauth && stats_lastmillis - a->lastauth < 60*1000)
                {
                    vector<char> buf;
                    //answerchallenge(a->key, text, buf);
                    //conoutf("answering %u, challenge %s with %s", id, text, buf.getbuf());
                    //addmsg(SV_AUTHANS, "rsis", a->desc, id, buf.getbuf());
                }
                break;
            }

            // :for AUTH

            case SV_GIBDAMAGE:
            case SV_DAMAGE:
            {
                int tcn = getint(p),
                    acn = getint(p),
                    gun = getint(p),
                    damage = getint(p),
                    armour = getint(p),
                    health = getint(p);
                playerent *target = stats_getclient(tcn), *actor = stats_getclient(acn);
                if(!target || !actor) break;
                target->armour = armour;
                target->health = health;
                //dodamage(damage, target, actor, -1, type==SV_GIBDAMAGE, false);
                //actor->pstatdamage[gun]+=damage; //NEW
                
                
                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("target",tcn);
                outdata.set("actor",acn);
                outdata.set("damage",damage);
                outdata.set("armour",armour);
                outdata.set("health",health);
                outdata.set("gun",gun);
                outdata.set("powershot",(bool)(type==SV_GIBDAMAGE));
                
                
                break;
            }

            case SV_POINTS:
            {
                int count = getint(p);
                if ( count > 0 ) {
                    loopi(count){
                        int pcn = getint(p); int score = getint(p);
                        playerent *ppl = stats_getclient(pcn);
                        if (!ppl) break;
                        ppl->points += score;
                    }
                } else {
                    int medals = getint(p);
                    if(medals > 0) {
//                         medals_arrived=1;
                        loopi(medals) {
                            int mcn=getint(p); int mtype=getint(p); int mitem=getint(p);
                            //a_medals[mtype].assigned=1;
                            //a_medals[mtype].cn=mcn;
                            //a_medals[mtype].item=mitem;
                        }
                    }
                }
                break;
            }

            case SV_HUDEXTRAS:
            {
                char value = getint(p);
                //if (hudextras) showhudextras(hudextras, value);
                break;
            }

            case SV_HITPUSH:
            {
                int gun = getint(p), damage = getint(p);
                vec dir;
                loopk(3) dir[k] = getint(p)/DNF;
                //stats_player1->hitpush(damage, dir, NULL, gun);

                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("gun",gun);
                outdata.set("damage",damage);
                outdata.set("dir.x",dir[0]);
                outdata.set("dir.y",dir[1]);
                outdata.set("dir.z",dir[2]);
                 
                break;
            }

            case SV_GIBDIED:
            case SV_DIED:
            {
                int vcn = getint(p), acn = getint(p), frags = getint(p), gun = getint(p);
                
                
                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("victim",vcn);
                outdata.set("actor",acn);
                outdata.set("frags",frags);
                outdata.set("gun",gun);
                outdata.set("powershot", (bool)(type==SV_GIBDIED));
                //playerent *victim = stats_getclient(vcn), *actor = stats_getclient(acn);
                //if(!actor) break;
                //if ( m_mp(gamemode) ) actor->frags = frags;
                //if(!victim) break;
                //dokill(victim, actor, type==SV_GIBDIED, gun);
                break;
            }

            case SV_RESUME:
            {
                loopi(MAXCLIENTS)
                {
                    int cn = getint(p);
                    if(p.overread() || cn<0) break;
                    int state = getint(p), lifesequence = getint(p), primary = getint(p), gunselect = getint(p), flagscore = getint(p), frags = getint(p), deaths = getint(p), health = getint(p), armour = getint(p), points = getint(p);
                    int teamkills = -1;
                    if(!demo || !stats_watchingdemo || stats_demoprotocol > 1132) teamkills = getint(p);
                    int ammo[NUMGUNS], mag[NUMGUNS];
                    loopi(NUMGUNS) ammo[i] = getint(p);
                    loopi(NUMGUNS) mag[i] = getint(p);
                    
                    playerent *d = (cn == stats_getclientnum() ? stats_player1 : stats_newclient(cn));
                    
                    
                    
                    dict_t outdata(event_to_str(type));
                    outdata.set("cn", cn0);
                    outdata.set("typestr", event_to_str(type));
                    outdata.set("ccn", cn);
                    outdata.set("state", state);
                    outdata.set("lifesequence", lifesequence);
                    outdata.set("primary", primary);
                    outdata.set("gunselect", gunselect);
                    outdata.set("flagscore", flagscore);
                    outdata.set("frags", frags);
                    outdata.set("deaths", deaths);
                    outdata.set("health", health);
                    outdata.set("armour", armour);
                    outdata.set("points", points);
                    
                    if(!d) continue;
                    /*
                    if(d!=player1) d->state = state;
                    d->lifesequence = lifesequence;
                    d->flagscore = flagscore;
                    d->frags = frags;
                    d->deaths = deaths;
                    d->points = points;
                    d->tks = teamkills;
                    if(d!=player1)
                    {
                        d->setprimary(primary);
                        d->selectweapon(gunselect);
                        d->health = health;
                        d->armour = armour;
                        memcpy(d->ammo, ammo, sizeof(ammo));
                        memcpy(d->mag, mag, sizeof(mag));
                        if(d->lifesequence==0) d->resetstats(); //NEW
                    }
                    */
                }
                break;
            }
            
            
            
            
            
            
            
            
            
            
            case SV_DISCSCORES:
            {
                //discscores.shrink(0);
                int team;
                while((team = getint(p)) >= 0)
                {
                    //discscore &ds = discscores.add();
                    //ds.team = team;
                    getstring(text, p);
                    //filtertext(ds.name, text, FTXT__PLAYERNAME, MAXNAMELEN);
                    int flags = getint(p);
                    int frags = getint(p);
                    int deaths = getint(p);
                    int points = getint(p);
                }
                break;
            }
            case SV_ITEMSPAWN:
            {
                int i = getint(p);
                //setspawn(i, true);
                    dict_t outdata(event_to_str(type));
                    outdata.set("cn", cn0);
                    outdata.set("typestr", event_to_str(type));
                    outdata.set("i", i);
                    
                break;
            }

            case SV_ITEMACC:
            {
                int i = getint(p), cn = getint(p);
                //playerent *d = stats_getclient(cn);
                //pickupeffects(i, d);
                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("ccn", cn);
                outdata.set("i", i);

                break;
            }

            case SV_EDITH:              // coop editing messages, should be extended to include all possible editing ops
            case SV_EDITT:
            case SV_EDITS:
            case SV_EDITD:
            case SV_EDITE:
            {
                int x  = getint(p);
                int y  = getint(p);
                int xs = getint(p);
                int ys = getint(p);
                int v  = getint(p);
                block b = { x, y, xs, ys };
                
                switch(type)
                {
                    case SV_EDITH: /*editheightxy(v!=0, getint(p), b);*/ getint(p); break;
                    case SV_EDITT: /*edittexxy(v, getint(p), b);*/ getint(p); break;
                    //case SV_EDITS: edittypexy(v, b); break;
                    //case SV_EDITD: setvdeltaxy(v, b); break;
                    //case SV_EDITE: editequalisexy(v!=0, b); break;
                }
                break;
            }

            case SV_NEWMAP:
            {
                int size = getint(p);
                //if(size>=0) empty_world(size, true);
                //else empty_world(-1, true);
                //if(d && d!=player1)
                    //conoutf(size>=0 ? _("%s started a new map of size %d") : _("%s enlarged the map to size %d"), colorname(d), sfactor);
                break;
            }

            case SV_EDITENT:            // coop edit of ent
            {
                uint i = getint(p);
                
                /*
                while((uint)ents.length()<=i) ents.add().type = NOTUSED;
                int to = ents[i].type;
                if(ents[i].type==SOUND)
                {
                    entity &e = ents[i];

                    entityreference entref(&e);
                    location *loc = audiomgr.locations.find(e.attr1, &entref, mapsounds);

                    if(loc)
                        loc->drop();
                }*/

                int type = getint(p);
                int x = getint(p);
                int y = getint(p);
                int z = getint(p);
                int attr1 = getint(p);
                int attr2 = getint(p);
                int attr3 = getint(p);
                int attr4 = getint(p);
                bool spawned = false;
                //if(ents[i].type==LIGHT || to==LIGHT) calclight();
                //if(ents[i].type==SOUND) audiomgr.preloadmapsound(ents[i]);
                break;
            }

            case SV_PONG:
            {
                int millis = getint(p);
                
                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("millis", millis);
                    
                //addmsg(SV_CLIENTPING, "i", stats_player1->ping = max(0, (player1->ping*5+totalmillis-millis)/6));
                break;
            }

            case SV_CLIENTPING:
            {
                int ping = getint(p);
                
                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("ping", ping);
                
                break;
            }
            case SV_GAMEMODE:
            {
                int nextmode = getint(p);
                //if (nextmode >= GMODE_NUM) nextmode -= GMODE_NUM;
                break;
            }
            case SV_TIMEUP:
            {
                int curgamemillis = getint(p);
                int curgamelimit = getint(p);
                
                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("curgamemillis", curgamemillis);
                outdata.set("curgamelimit", curgamelimit);
                
                //timeupdate(curgamemillis, curgamelimit);
                break;
            }

            case SV_WEAPCHANGE:
            {
                int gun = getint(p);
                //if(d) d->selectweapon(gun);
                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("gun", gun);
                break;
            }

            case SV_SERVMSG:
            {
                getstring(text, p);
                
                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("text", (const char*)text);
                
                //conoutf("%s", text);
                break;
            }
            
            case SV_FLAGINFO:
            {
                int flag = getint(p);
                //if(flag<0 || flag>1) return;
                //flaginfo &f = flaginfos[flag];
                int state = getint(p);
                switch(state)
                {
                    case CTFF_STOLEN:
                        getint(p);// flagstolen(flag, getint(p));
                        break;
                    case CTFF_DROPPED:
                    {
                        float x = getuint(p)/DMF;
                        float y = getuint(p)/DMF;
                        float z = getuint(p)/DMF;
                        //flagdropped(flag, x, y, z);
                        break;
                    }
                    case CTFF_INBASE:
                        //flaginbase(flag);
                        break;
                    case CTFF_IDLE:
                        //flagidle(flag);
                        break;
                }
                break;
            }

            case SV_FLAGMSG:
            {
                int flag = getint(p);
                int message = getint(p);
                int actor = getint(p);
                int flagtime = message == FM_KTFSCORE ? getint(p) : -1;
                
                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("flag", flag);
                outdata.set("message", message);
                outdata.set("actor", actor);
                outdata.set("flagtime", flagtime);
                
                //flagmsg(flag, message, actor, flagtime);
                break;
            }

            case SV_FLAGCNT:
            {
                int fcn = getint(p);
                int flags = getint(p);
                
                
                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("fcn", fcn);
                outdata.set("flags", flags);
                
                //playerent *p = stats_getclient(fcn);
                //if(p) p->flagscore = flags;
                break;
            }

            case SV_ARENAWIN:
            {
                int acn = getint(p);
                
                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("acn", acn);
                

                /*
                playerent *alive = getclient(acn);
                conoutf(_("the round is over! next round in 5 seconds..."));
                if(m_botmode && acn==-2) hudoutf(_("the bots have won the round!"));
                else if(!alive) hudoutf(_("everyone died!"));
                else if(m_teammode) hudoutf(_("team %s has won the round!"), team_string(alive->team));
                else if(alive==player1) hudoutf(_("you are the survivor!"));
                else hudoutf(_("%s is the survivor!"), colorname(alive));
                arenaintermission = stats_lastmillis;
                */
                break;
            }

            case SV_SPAWNDENY:
            {
                //extern int spawnpermission;
                int spawnpermission = getint(p);
                //if(spawnpermission == SP_REFILLMATCH) hudoutf("\f3You can now spawn to refill your team.");
                break;
            }
            case SV_FORCEDEATH:
            {
                int cn = getint(p);
                
                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("ccn", cn);
                
                
                playerent *d = cn==stats_getclientnum() ? stats_player1 : stats_newclient(cn);
                if(!d) break;
                //deathstate(d);
                break;
            }

            case SV_SERVOPINFO:
            {
                /*
                loopv(players) { if(stats_players[i]) stats_players[i]->clientrole = CR_DEFAULT; }
                stats_player1->clientrole = CR_DEFAULT;
                */
                int cl = getint(p), r = getint(p);
                if(cl >= 0 && r >= 0)
                {
                    playerent *pl = (cl == stats_getclientnum() ? stats_player1 : stats_newclient(cl));
                    //if(pl)
                    {
                        //pl->clientrole = r;
                        if(pl->name[0])
                        {
                            //// two messages required to allow for proper german translation - is there a better way to do it?
                            //if(pl==player1) conoutf(_("you claimed %s status"), r == CR_ADMIN ? "admin" : "master");
                            //else conoutf(_("%s claimed %s status"), colorname(pl), r == CR_ADMIN ? "admin" : "master");
                        }
                    }
                }
                
                break;
            }

            case SV_TEAMDENY:
            {
                int t = getint(p);

                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("team", t);
                
                //if(m_teammode)
                //{
                    //if(team_isvalid(t)) conoutf(_("you can't change to team %s"), team_string(t));
                //}
                //else
                {
                    //if(team_isspect(t)) conoutf(_("you can't change to spectate mode"));
                    //else if (player1->state!=CS_ALIVE) conoutf(_("you can't change to active mode"));
                    //else conoutf(_("you can't switch teams while being alive"));
                }
                break;
            }
            
            
            
            
            
            case SV_SETTEAM:
            {
                int fpl = getint(p), fnt = getint(p), ftr = fnt >> 4;
                fnt &= 0x0f;
                playerent *d = (fpl == stats_getclientnum() ? stats_player1 : stats_newclient(fpl));
                
                const char* ftr_str = (ftr == FTR_PLAYERWISH) ? "FTR_PLAYERWISH" : "FTR_AUTOTEAM";
                
                
                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("ccn", fpl);
                outdata.set("new_team", fnt);
                outdata.set("ftr", ftr_str);
                /*
                if(d)
                {
                    const char *nts = team_string(fnt);
                    bool you = fpl == stats_player1->clientnum;
                    if(m_teammode || team_isspect(fnt))
                    {
                        if(d->team == fnt)
                        {
                            if(you && ftr == FTR_AUTOTEAM) hudoutf("you stay in team %s", nts);
                        }
                        else
                        {
                            if(you && !stats_watchingdemo)
                            {
                                switch(ftr)
                                {
                                    case FTR_PLAYERWISH:
                                        conoutf(_("you're now in team %s"), nts);
                                        break;
                                    case FTR_AUTOTEAM:
                                        hudoutf(_("the server forced you to team %s"), nts);
                                        break;
                                }
                            }
                            else
                            {
                                const char *pls = colorname(d);
                                bool et = team_base(player1->team) != team_base(fnt);
                                switch(ftr)
                                {
                                    case FTR_PLAYERWISH:
                                        conoutf(_("player %s switched to team %s"), pls, nts); // new message
                                        break;
                                    case FTR_AUTOTEAM:
                                        if(stats_watchingdemo) conoutf(_("the server forced %s to team %s"), colorname(d), nts);
                                        else hudoutf(_("the server forced %s to %s team"), colorname(d), et ? _("the enemy") : _("your"));
                                        break;
                                }
                            }
                            if(you && !team_isspect(d->team) && team_isspect(fnt) && d->state == CS_DEAD) spectatemode(SM_FLY);
                        }
                    }
                    else if(d->team != fnt && ftr == FTR_PLAYERWISH && !team_isactive(d->team)) conoutf(_("%s changed to active play"), you ? _("you") : colorname(d));
                    d->team = fnt;
                    if(team_isspect(d->team)) d->state = CS_SPECTATE;
                }
                */
                break;
            }

            case SV_SERVERMODE:
            {
                int sm = getint(p);
                //servstate.autoteam = sm & 1;
                //servstate.mastermode = (sm >> 2) & MM_MASK;
                //servstate.matchteamsize = sm >> 4;
                //if(sm & AT_SHUFFLE) playsound(TEAMSHUFFLE);    // TODO
                break;
            }

            case SV_CALLVOTE:
            {
                int vote_type = getint(p);
                int vcn = -1, n_yes = 0, n_no = 0;
                if ( vote_type == -1 )
                {
                    d = stats_getclient(vcn);
                    n_yes = getint(p);
                    n_no = getint(p);
                    vote_type = getint(p);
                }
                
                if (vote_type == SA_MAP && d == NULL) d = stats_player1;      // gonext uses this
                
                if( vote_type < 0 || vote_type >= SA_NUM || !d ) return;
                //votedisplayinfo *v = NULL;
                string a1, a2;
                switch(vote_type)
                {
                    case SA_MAP:
                    {
                        getstring(text, p);
                        int mode = getint(p);
                        //if(m_isdemo(mode)) filtertext(text, text, FTXT__DEMONAME);
                        //else filtertext(text, behindpath(text), FTXT__MAPNAME);
                        itoa(a1, mode);
                        defformatstring(t)("%d", getint(p));
                        //v = newvotedisplayinfo(d, vote_type, text, a1, t);
                        break;
                    }
                    case SA_KICK:
                    case SA_BAN:
                    {
                        itoa(a1, getint(p));
                        getstring(text, p);
                        filtertext(text, text, FTXT__CHAT);
                        //v = newvotedisplayinfo(d, vote_type, a1, text);
                        break;
                    }
                    case SA_SERVERDESC:
                        getstring(text, p);
                        filtertext(text, text, FTXT__SERVDESC);
                        //v = newvotedisplayinfo(d, vote_type, text, NULL);
                        break;
                    case SA_STOPDEMO:
                        // compatibility
                        break;
                    case SA_REMBANS:
                    case SA_SHUFFLETEAMS:
                        //v = newvotedisplayinfo(d, vote_type, NULL, NULL);
                        break;
                    case SA_FORCETEAM:
                        itoa(a1, getint(p));
                        itoa(a2, getint(p));
                        //v = newvotedisplayinfo(d, vote_type, a1, a2);
                        break;
                    default:
                        itoa(a1, getint(p));
                        //v = newvotedisplayinfo(d, vote_type, a1, NULL);
                        break;
                }
                //displayvote(v);
                //onCallVote(vote_type, v->owner->clientnum, text, a1);
                //if (vcn >= 0)
                //{
                //    loopi(n_yes) votecount(VOTE_YES);
                //    loopi(n_no) votecount(VOTE_NO);
                //}
                //extern int vote(int);
                //if (d == stats_player1) vote(VOTE_YES);
                break;
            }

            case SV_CALLVOTESUC:
            {
                //callvotesuc();
                //onChangeVote( 0, -1);
                break;
            }

            case SV_CALLVOTEERR:
            {
                int errn = getint(p);
                //callvoteerr(errn);
                //onChangeVote( 1, errn);
                break;
            }

            case SV_VOTE:
            {
                int vote = getint(p);
                //votecount(vote);
                //onChangeVote( 2, vote);
                break;
            }

            case SV_VOTERESULT:
            {
                int vres = getint(p);
                //voteresult(vres);
                //onChangeVote( 3, vres);
                break;
            }

            case SV_IPLIST:
            {
                int cn;
                while((cn = getint(p)) >= 0 && !p.overread())
                {
                    playerent *pl = stats_getclient(cn);
                    int ip = getint(p);
                    if(!pl) continue;
                    else pl->address = ip;
                }
                break;
            }

            case SV_SENDDEMOLIST:
            {
                int demos = getint(p);
                //menureset(downloaddemomenu);
                //demo_mlines.shrink(0);
                if(!demos)
                {
                    //conoutf(_("no demos available"));
                    //mline &m = demo_mlines.add();
                    //copystring(m.name, "no demos available");
                    //menumanual(downloaddemomenu,m.name);
                }
                else
                {
                    //demo_mlines.reserve(demos);
                    loopi(demos)
                    {
                        getstring(text, p);
                        //conoutf("%d. %s", i+1, text);
                        //mline &m = demo_mlines.add();
                        //formatstring(m.name)("%d. %s", i+1, text);
                        //formatstring(m.cmd)("getdemo %d", i+1);
                        //menumanual(downloaddemomenu, m.name, m.cmd);
                    }
                }
                break;
            }

            case SV_DEMOPLAYBACK:
            {
                string demofile;
                //extern char *curdemofile;
                if(demo && stats_watchingdemo && stats_demoprotocol == 1132)
                {
                    stats_watchingdemo = demoplayback = getint(p)!=0;
                    copystring(demofile, "n/a");
                }
                else
                {
                    getstring(demofile, p, MAXSTRLEN);
                    stats_watchingdemo = demoplayback = demofile[0] != '\0';
                }
                //DELETEA(curdemofile);
                if(demoplayback)
                {
                    //curdemofile = newstring(demofile);
                    //player1->resetspec();
                    //player1->state = CS_SPECTATE;
                    //player1->team = TEAM_SPECT;
                }
                else
                {
                    // cleanups
                    //curdemofile = newstring("n/a");
                    //loopv(players) zapplayer(players[i]);
                    //clearvote();
                    //player1->state = CS_ALIVE;
                    //player1->resetspec();
                }
                int clientnum = getint(p);
                
                dict_t outdata(event_to_str(type));
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                outdata.set("demofile", (const char*)demofile);
                outdata.set("clientnum", clientnum);
                
                break;
            }
            

            default:
            {
                dict_t outdata("network-error");
                outdata.set("cn", cn0);
                outdata.set("typestr", event_to_str(type));
                
                neterr("type");
                return;
            }

        }
    }
    
    
    #ifdef _DEBUG
    protocoldebug(false);
    #endif

}


playerent *stats_newclient(int cn)   // ensure valid entity
{
    if(cn<0 || cn>=MAXCLIENTS)
    {
        neterr("clientnum");
        return NULL;
    }
    while(cn>=stats_players.length()) stats_players.add(NULL);
    playerent *d = stats_players[cn];
    if(d) return d;
    d = new playerent;
    stats_players[cn] = d;
    d->clientnum = cn;
    return d;
}

playerent *stats_getclient(int cn)   // ensure valid entity
{
    if(cn == stats_player1->clientnum) return stats_player1;
    return stats_players.inrange(cn) ? stats_players[cn] : NULL;
}

int stats_getclientnum() { return stats_player1 ? stats_player1->clientnum : -1; }


void rstats()
{
    stats_startdemoplayback("20140929_1814_64.85.165.122_TwinTowers_2013_Patched_7min_TOSOK");
    stats_readdemo();
}








