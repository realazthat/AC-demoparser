
#include "cube.h"

void fatal(const char *s, ...)    // failure exit
{

    throw std::runtime_error("stats::fatal()");
}

bool interceptkey(int sym)
{
    throw std::runtime_error("stats::interceptkey()");
    return false;
}




int variable(const char *name, int min, int cur, int max, int *storage, void (*fun)(), bool persist)
{
    //fprintf(stderr,"variable(name=\"%s\")\n", name);

    return 0;
}
bool addcommand(const char *name, void (*fun)(), const char *sig)
{
    //fprintf(stderr,"addcommand(name=\"%s\")\n", name);
    
    return true;
}

void clientlogf(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    vfprintf(stderr,format,args);
    va_end(args);
    fputs("\n",stderr);

}
void conoutf(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    vfprintf(stderr,format,args);
    va_end(args);
    fputs("\n",stderr);
}


bool identexists(const char *name)
{
    return false;
}

void neterr(const char* s)
{
    fputs(s,stderr);
    fputs("\n",stderr);
}

authkey *findauthkey(const char *desc)
{
    return NULL;
}

bool weapon::valid(int id) { return id>=0 && id<NUMGUNS; }

itemstat ammostats[NUMGUNS];
guninfo guns[NUMGUNS];



void removebounceents(playerent*)
{
    
}
void zapplayerflags(playerent*){}
void cleanplayervotes(playerent*){}
void togglespect(){}

audiomanager audiomgr;
audiomanager::audiomanager(){}
void audiomanager::detachsounds(playerent*){}
void removedynlights(physent*){}
physent *camera1;

int lastmillis;

const int weapon::weaponchangetime = 0;
bufferhashtable::~bufferhashtable(){}
sbuffer* bufferhashtable::find(char*){
    return NULL;
}

sbuffer::~sbuffer(){}
//#include "server.h"
