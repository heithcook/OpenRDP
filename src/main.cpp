#include "app/Application.h"
#include <freerdp/freerdp.h>
#include <QtCore/qglobal.h>
#include <cstdio>
#include <cstring>
int main(int argc,char** argv){
    for(int i=1;i<argc;++i) if(std::strcmp(argv[i],"--version")==0){
        std::printf("OpenRDP 0.2.0\nFreeRDP %s\nQt %s\nLinux %s\n",freerdp_get_version_string(),qVersion(),sizeof(void*)==8?"x86_64":"unknown");
        return 0;
    }
    return openrdp::runApplication(argc,argv);
}
