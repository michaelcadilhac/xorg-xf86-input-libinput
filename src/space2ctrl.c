#include <sys/time.h>

#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <xorg-server.h>
#include <list.h>
#include <exevents.h>
#include <xkbsrv.h>
#include <xf86.h>
#include <xf86Xinput.h>
#include <xf86_OSproc.h>
#include <xserver-properties.h>
#include <libinput.h>
#include <linux/input.h>
#include <sys/time.h>

#include <X11/Xatom.h>
#include <signal.h>

#include "space2ctrl.h"

#define SPACE_KEY 65
#define CTRLL_KEY 37
#define BACKUPSPACE_KEY 108

typedef enum {
  SPACE_IS_NOT_CTRL,
  SPACE_MAY_BE_CTRL,
  SPACE_IS_CTRL
} SpaceState;


static inline void postCtrl(DeviceIntPtr dev, int pressCtrl)
{
  xf86PostKeyboardEvent(dev, CTRLL_KEY, pressCtrl);
}

static inline int diff_ms(struct timeval t1, struct timeval t2) {
  return ( ((t1.tv_sec - t2.tv_sec) * 1000000)
           + (t1.tv_usec - t2.tv_usec) ) / 1000;
}

static void space2ctrlSend(InputInfoPtr pInfo, int key, int isPress, int isMouse)
{
  DeviceIntPtr dev = pInfo->dev;
  static int translatedBackup = 0;
  static int waitKey = 0;
  static int waitIsMouse = 0;
  static int nPressedKeys = 0;
  static SpaceState spaceState = SPACE_IS_NOT_CTRL;
  static struct timeval startWait, endWait;

  //  xf86IDrvMsg(pInfo, X_WARNING, "Driver: %d %d %d\n", key, isPress, isMouse);
  if (isPress) // PRESS
  {
    if (key == SPACE_KEY && !isMouse && nPressedKeys == 0)
    {
      spaceState = SPACE_MAY_BE_CTRL;
      gettimeofday(&startWait, NULL);
    }
    else
    {
      if (spaceState == SPACE_MAY_BE_CTRL && nPressedKeys > 1)
      {
        // Space + at least two keys: space is control
        spaceState = SPACE_IS_CTRL;
        postCtrl(dev, 1);
        if (waitIsMouse)
          xf86PostButtonEvent(dev, Relative, waitKey, 1, 0, 0);
        else
          xf86PostKeyboardEvent(dev, waitKey, 1);
        waitKey = 0;
      }
      if (spaceState == SPACE_MAY_BE_CTRL && nPressedKeys == 1)
      {
        // Space + this key, delay it
        waitKey = key;
        waitIsMouse = isMouse;
      }
      if (spaceState != SPACE_MAY_BE_CTRL)
      {
        if (spaceState == SPACE_IS_CTRL && key == BACKUPSPACE_KEY && !isMouse)
        {
          key = SPACE_KEY;
          translatedBackup = 1;
        }
        if (isMouse)
          xf86PostButtonEvent(dev, Relative, key, 1, 0, 0);
        else
          xf86PostKeyboardEvent(dev, key, 1);
      }
    }
    nPressedKeys++;
  }
  else // RELEASE
  {
    if (key == SPACE_KEY && spaceState != SPACE_IS_NOT_CTRL)
    {
      if (spaceState == SPACE_IS_CTRL)
        postCtrl(dev, 0);
      else
      {
        gettimeofday(&endWait, NULL);
        if (diff_ms(endWait, startWait) < 600) {
          if (isMouse) {
            xf86PostButtonEvent(dev, Relative, key, 1, 0, 0);
            xf86PostButtonEvent(dev, Relative, key, 0, 0, 0);
          }
          else {
            xf86PostKeyboardEvent(dev, key, 1);
            xf86PostKeyboardEvent(dev, key, 0);
          }
        }
        // A key was delayed because we did not know if space was CTRL, now we
        // know it's not.
        if (waitKey) {
          if (waitIsMouse)
            xf86PostButtonEvent(dev, Relative, waitKey, 1, 0, 0);
          else
            xf86PostKeyboardEvent(dev, waitKey, 1);
          waitKey = 0;
        }
      }
      spaceState = SPACE_IS_NOT_CTRL;
    }
    else // Not SPACE_KEY
    {
      if (translatedBackup && key == BACKUPSPACE_KEY)
      {
        key = SPACE_KEY;
        isMouse = 0;
        translatedBackup = 0;
      }
      if (spaceState == SPACE_MAY_BE_CTRL)
      {
        spaceState = SPACE_IS_CTRL;
        postCtrl(dev, 1);
        if (waitKey != key || waitIsMouse != isMouse)
          // Here, waitKey should be key, since Space + two keys triggers
          // SPACE_IS_CTRL.
          xf86IDrvMsg(pInfo, X_ERROR, "Unexpected value of waitKey\n");
        if (key == BACKUPSPACE_KEY && !isMouse)
        {
          key = SPACE_KEY;
          isMouse = 0;
        }
        if (isMouse)
          xf86PostButtonEvent(dev, Relative, key, 1, 0, 0);
        else
          xf86PostKeyboardEvent(dev, key, 1);
        waitKey = 0;
      }
      if (isMouse)
        xf86PostButtonEvent(dev, Relative, key, 0, 0, 0);
      else
        xf86PostKeyboardEvent(dev, key, 0);
    }
    if (nPressedKeys > 0)
      nPressedKeys--;
  }
}

void space2ctrlKeySend(InputInfoPtr pInfo, int key, int isPress)
{
  space2ctrlSend(pInfo, key, isPress, 0);
}

void space2ctrlButtonSend(InputInfoPtr pInfo, int button, int isPress)
{
  space2ctrlSend(pInfo, button, isPress, 1);
}
