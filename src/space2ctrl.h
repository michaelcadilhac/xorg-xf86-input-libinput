#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef SPACE2CTRL_H
#define SPACE2CTRL_H

#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <xorg-server.h>
#include <list.h>
#include <exevents.h>
#include <xkbsrv.h>

void space2ctrlKeySend(InputInfoPtr pInfo, int key, int isPress);
void space2ctrlButtonSend(InputInfoPtr pInfo, int button, int isPress);

#endif
